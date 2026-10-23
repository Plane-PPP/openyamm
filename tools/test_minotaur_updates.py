#!/usr/bin/env python3
"""Exercise offline updates; on Windows run the real Windows PowerShell5.1 installer."""
import copy
import hashlib
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
import zipfile

from fetch_minotaur_baseline import RemoteZip, extract_baseline
from minotaur_updates import (build_update, digest_file, index_identity, make_baseline, plan_update,
                              read_json, resolve_index, safe_relative, validate_index, write_json)


def pack(folder, package, data, compression=zipfile.ZIP_STORED):
    identity = hashlib.sha256(data).hexdigest()
    name = package + '.core-' + identity[:16] + '.zip'
    path = folder / 'packs' / name
    path.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(path, 'w', compression=compression) as archive:
        archive.writestr('content.bin', data)
    return {'id': package + '.core', 'package': package,
            'mount': '/engine' if package == 'engine' else '/worlds/' + package,
            'file': name, 'size': path.stat().st_size, 'sha256': digest_file(path), 'content_id': identity}


def release(root, version, *, engine=b'old-table', compressed=False, program=b'new-program'):
    stage, content = root / version / 'stage', root / version / 'content'
    app = stage / 'app' / version
    app.mkdir(parents=True)
    (stage / 'app/current').write_text(version + '\n')
    (app / 'openyamm.exe').write_bytes(program)
    (app / 'OpenYAMM-launcher.exe').write_bytes(b'new-launcher')
    (app / 'runtime').mkdir()
    (app / 'runtime/shader.bin').write_bytes(b'shader')
    # A large unchanged resource makes accidental resource inclusion visible in the ZIP test.
    records = [pack(content, name, engine if name == 'engine' else (name.encode() * (1024 * 512)),
                    zipfile.ZIP_DEFLATED if compressed else zipfile.ZIP_STORED)
               for name in ['engine', 'mm6', 'mm7', 'mm8']]
    index = {'schema': 1, 'profile': 'desktop', 'content_format': 1, 'store': '../../assets/packs',
             'release': version, 'build': 'fixture', 'packs': records}
    write_json(app / 'index.json', index)
    return stage, content, make_baseline(version, 'a' * 40, index)


class Fixture(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.base_stage, self.base_content, self.base = release(self.root, 'minotaur-1.10', program=b'old')

    def tearDown(self):
        self.temporary.cleanup()

    def update(self, version='minotaur-1.10-new', base=None, **options):
        stage, content, target = release(self.root, version, **options)
        output = self.root / (version + '.zip')
        manifest = build_update(stage, content, base or self.base, 'b' * 40, output,
                                self.root / (version + '-baseline.json'))
        extracted = self.root / (version + '-extracted')
        with zipfile.ZipFile(output) as archive:
            archive.extractall(extracted)
        return manifest, output, next(extracted.iterdir())


class PlannerTests(Fixture):
    def test_program_only_update_omits_all_heavy_packs(self):
        manifest, archive_path, _ = self.update()
        self.assertEqual(len(manifest['reused_packs']), 4)
        self.assertEqual(manifest['changed_packs'], [])
        self.assertLess(archive_path.stat().st_size, 100000)
        with zipfile.ZipFile(archive_path) as archive:
            self.assertFalse(any('/payload/packs/' in n for n in archive.namelist()))

    def test_changed_table_ships_only_engine_pack(self):
        manifest, archive_path, _ = self.update(engine=b'new-table')
        self.assertEqual(manifest['changed_packs'], ['engine.core'])
        with zipfile.ZipFile(archive_path) as archive:
            packs = [n for n in archive.namelist() if '/payload/packs/' in n]
            self.assertEqual(len(packs), 1)
            self.assertIn('engine.core-', packs[0])

    def test_compressor_change_reuses_original_bytes_and_future_base(self):
        manifest, _, _ = self.update(compressed=True)
        self.assertEqual(manifest['changed_packs'], [])
        target = manifest['target']['index']
        resolved = resolve_index(self.base['index'], target, manifest['reused_packs'])
        self.assertNotEqual(target['packs'][1]['sha256'], resolved['packs'][1]['sha256'])
        self.assertEqual(index_identity(target), index_identity(resolved))
        for old, actual in zip(self.base['index']['packs'], resolved['packs']):
            self.assertEqual(old['sha256'], actual['sha256'])

    def test_removal_does_not_include_removed_pack(self):
        # Drop a domain pack, keeping every required world present.
        extra = copy.deepcopy(self.base['index']['packs'][0])
        extra.update(id='engine.extra', file='engine.extra.zip')
        index = copy.deepcopy(self.base['index']); index['packs'].append(extra)
        base = make_baseline('minotaur-old', 'a' * 40, index)
        reused, changed = plan_update(base, self.base)
        self.assertEqual(len(reused), 4)
        self.assertEqual(changed, [])

    def test_reject_same_version_and_corrupt_payload(self):
        with self.assertRaises(ValueError):
            plan_update(self.base, self.base)
        stage, content, _ = release(self.root, 'minotaur-bad', engine=b'new-table')
        next((content / 'packs').glob('engine*')).write_bytes(b'corrupt')
        with self.assertRaises(ValueError):
            build_update(stage, content, self.base, 'b' * 40, self.root / 'bad.zip', self.root / 'bad.json')

    def test_reject_unsafe_paths_missing_world_and_duplicate_files(self):
        for name in ['../settings.ini', '/saves/a', 'C:/x', 'a\\b', 'a/../b', 'CON.txt', 'a./b', 'a /b']:
            with self.subTest(name=name), self.assertRaises(ValueError):
                safe_relative(name)
        bad = copy.deepcopy(self.base['index']);bad['packs'].pop()
        with self.assertRaises(ValueError): validate_index(bad)
        bad = copy.deepcopy(self.base['index']);bad['packs'][1]['file'] = bad['packs'][0]['file'].upper()
        with self.assertRaises(ValueError): validate_index(bad)


class RangeZipTests(Fixture):
    def test_metadata_read_does_not_fetch_heavy_payload(self):
        stream = io.BytesIO()
        with zipfile.ZipFile(stream, 'w', compression=zipfile.ZIP_STORED) as archive:
            with archive.open('OpenYAMM/assets/large.zip', 'w', force_zip64=True) as target:
                target.write(b'x' * (8 * 1024 * 1024))
            archive.writestr('OpenYAMM/app/current', self.base['version'])
            archive.writestr('OpenYAMM/BUILD-REVISION.txt', 'Commit SHA: ' + 'a' * 40 + '\n')
            archive.writestr('OpenYAMM/app/' + self.base['version'] + '/index.json', json.dumps(self.base['index']))
        data = stream.getvalue()
        class MemoryRange(RemoteZip):
            def fetch(self, start, end):
                self.downloaded += end - start + 1
                return data[start:end + 1]
        remote = MemoryRange('not-a-network-url', len(data))
        actual = extract_baseline(remote, self.base['version'], 'a' * 40)
        self.assertEqual(actual['index_identity'], self.base['index_identity'])
        self.assertLess(remote.downloaded, 256 * 1024)
        with self.assertRaises(ValueError):
            extract_baseline(MemoryRange('', len(data)), self.base['version'], 'b' * 40)


@unittest.skipUnless(os.name == 'nt', 'Requires the real Windows PowerShell5.1 installer')
class WindowsInstallerTests(Fixture):
    def setUp(self):
        super().setUp()
        self.install = self.root / 'installed'
        shutil.copytree(self.base_stage, self.install)
        shutil.copytree(self.base_content / 'packs', self.install / 'assets/packs')
        (self.install / 'OpenYAMM.exe').write_bytes(b'old-launcher')
        (self.install / 'settings.ini').write_bytes(b'player-settings')
        (self.install / 'saves').mkdir()
        (self.install / 'saves/precious.oysav').write_bytes(b'player-progress')
        self.shell = shutil.which('powershell.exe')
        self.assertIsNotNone(self.shell)

    def apply(self, directory, success=True, check_only=False):
        args = [self.shell, '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
                str(directory / 'Apply-MinotaurUpdate.ps1'), '-InstallationRoot', str(self.install)]
        if check_only: args.append('-CheckOnly')
        result = subprocess.run(args, capture_output=True, timeout=120)
        self.assertEqual(result.returncode == 0, success, result.stdout.decode(errors='replace') +
                         result.stderr.decode(errors='replace'))
        self.assertEqual((self.install / 'settings.ini').read_bytes(), b'player-settings')
        self.assertEqual((self.install / 'saves/precious.oysav').read_bytes(), b'player-progress')

    def current(self):
        return (self.install / 'app/current').read_text(encoding='utf-8-sig').strip()

    def test_program_update_and_repeat_preserve_old_version_and_data(self):
        manifest, _, directory = self.update()
        self.apply(directory, check_only=True)
        self.assertEqual(self.current(), self.base['version'])
        self.assertFalse((self.install / 'update').exists())
        self.apply(directory)
        self.assertEqual(self.current(), manifest['target']['version'])
        self.assertEqual((self.install / 'app/previous').read_text().strip(), self.base['version'])
        self.assertTrue((self.install / 'app' / self.base['version'] / 'openyamm.exe').is_file())
        self.apply(directory)

    def test_patch_chain_handles_compression_variant_and_changed_table(self):
        first, _, directory = self.update(compressed=True)
        self.apply(directory)
        installed = read_json(self.install / 'app' / self.current() / 'index.json')
        self.assertEqual(installed['packs'][1]['sha256'], self.base['index']['packs'][1]['sha256'])
        second, _, directory = self.update('minotaur-next', base=first['target'], engine=b'changed', compressed=True)
        self.apply(directory)
        self.assertEqual(self.current(), second['target']['version'])

    def test_wrong_base_refused_before_switch(self):
        _, _, directory = self.update()
        (self.install / 'app/current').write_text('minotaur-unrelated\n')
        shutil.copytree(self.install / 'app' / self.base['version'], self.install / 'app/minotaur-unrelated')
        self.apply(directory, success=False)
        self.assertEqual(self.current(), 'minotaur-unrelated')

    def test_corrupt_reused_resource_refused_then_retry_succeeds(self):
        manifest, _, directory = self.update()
        record = self.base['index']['packs'][1]
        local = self.install / 'assets/packs' / record['file']
        original = local.read_bytes();local.write_bytes(b'broken')
        self.apply(directory, success=False)
        self.assertEqual(self.current(), self.base['version'])
        self.assertFalse((self.install / 'app' / manifest['target']['version']).exists())
        local.write_bytes(original)
        self.apply(directory)

    def test_corrupt_app_payload_refused(self):
        _, _, directory = self.update()
        (directory / 'payload/app/openyamm.exe').write_bytes(b'bad')
        self.apply(directory, success=False)
        self.assertEqual(self.current(), self.base['version'])

    def test_unsafe_payload_path_refused(self):
        _, _, directory = self.update()
        manifest = read_json(directory / 'minotaur-update.json')
        manifest['app_files'][0]['path'] = '../../settings.ini'
        write_json(directory / 'minotaur-update.json', manifest)
        self.apply(directory, success=False)
        self.assertEqual(self.current(), self.base['version'])

    def test_existing_pack_name_collision_does_not_overwrite_old_bytes(self):
        manifest, _, directory = self.update(engine=b'changed-table')
        record = manifest['target']['index']['packs'][0]
        old = self.install / 'assets/packs' / record['file']
        old.write_bytes(b'another-compression-variant')
        self.apply(directory)
        self.assertEqual(old.read_bytes(), b'another-compression-variant')
        installed = read_json(self.install / 'app' / self.current() / 'index.json')
        self.assertNotEqual(installed['packs'][0]['file'], record['file'])

    def test_resume_after_app_directory_staged_before_switch(self):
        manifest, _, directory = self.update()
        target = self.install / 'app' / manifest['target']['version']
        shutil.copytree(directory / 'payload/app', target)
        write_json(target / 'index.json', resolve_index(self.base['index'], manifest['target']['index'],
                                                       manifest['reused_packs']))
        self.apply(directory)
        self.assertEqual(self.current(), manifest['target']['version'])


if __name__ == '__main__':
    unittest.main(verbosity=2)
