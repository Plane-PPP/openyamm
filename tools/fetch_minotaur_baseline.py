#!/usr/bin/env python3
"""Read only the index/revision of a published ZIP, including ZIP64, using HTTP ranges."""
import argparse
import io
import json
from pathlib import Path
import re
import subprocess
import urllib.error
import urllib.request
import zipfile

from minotaur_updates import make_baseline, write_json


class RemoteZip(io.RawIOBase):
    block_size = 65536

    def __init__(self, url, size):
        self.url, self.size, self.position = url, size, 0
        self.cache = {}
        self.downloaded = 0

    def seekable(self):
        return True

    def readable(self):
        return True

    def tell(self):
        return self.position

    def seek(self, offset, whence=0):
        self.position = offset + (0 if whence == 0 else self.position if whence == 1 else self.size)
        if self.position < 0:
            raise ValueError("Negative seek")
        return self.position

    def fetch(self, start, end):
        request = urllib.request.Request(self.url, headers={"Range": f"bytes={start}-{end}"})
        try:
            with urllib.request.urlopen(request, timeout=60) as response:
                expected = f"bytes {start}-{end}/{self.size}"
                if response.status != 206 or response.headers.get("Content-Range") != expected:
                    raise ValueError("Artifact server did not honor byte ranges; refusing a full download")
                data = response.read(end - start + 2)
        except (urllib.error.URLError, TimeoutError):
            raise RuntimeError("Cannot read artifact byte range (URL withheld)") from None
        if len(data) != end - start + 1:
            raise ValueError("Incomplete artifact byte range")
        self.downloaded += len(data)
        return data

    def read(self, size=-1):
        remaining = max(0, self.size - self.position)
        size = remaining if size < 0 else min(size, remaining)
        if size > 4 * 1024 * 1024:
            raise ValueError("Refusing to read more than 4 MiB for baseline metadata")
        result = bytearray()
        while size:
            start = self.position // self.block_size * self.block_size
            if start not in self.cache:
                self.cache[start] = self.fetch(start, min(start + self.block_size, self.size) - 1)
            offset = self.position - start
            part = self.cache[start][offset:offset + size]
            result.extend(part)
            self.position += len(part)
            size -= len(part)
        return bytes(result)


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, request, fp, code, message, headers, new_url):
        return None


def artifact_url(repo, artifact_id):
    # Do not forward a GitHub authorization header to the signed blob URL.
    token = subprocess.run(["gh", "auth", "token", "--hostname", "github.com"],
                           check=True, capture_output=True, text=True).stdout.strip()
    request = urllib.request.Request(
        f"https://api.github.com/repos/{repo}/actions/artifacts/{artifact_id}/zip",
        headers={"Authorization": "Bearer " + token, "Accept": "application/vnd.github+json"})
    try:
        response = urllib.request.build_opener(NoRedirect()).open(request, timeout=60)
        response.close()
    except urllib.error.HTTPError as response:
        if response.code == 302:
            location = response.headers["Location"]
            response.close()
            if not location.startswith("https://"):
                raise ValueError("Artifact download must use HTTPS")
            return location
        response.close()
    raise RuntimeError("GitHub did not provide an artifact download redirect")


def extract_baseline(stream, version, source_sha):
    with zipfile.ZipFile(stream) as archive:
        current = archive.read("OpenYAMM/app/current").decode("utf-8-sig").strip()
        revision = archive.read("OpenYAMM/BUILD-REVISION.txt").decode("utf-8-sig")
        if current != version or not re.search(r"^Commit SHA: " + source_sha + r"\s*$", revision, re.M):
            raise ValueError("Published package revision differs from the expected baseline")
        name = "OpenYAMM/app/" + version + "/index.json"
        if archive.getinfo(name).file_size > 4 * 1024 * 1024:
            raise ValueError("Unreasonably large content index")
        index = json.loads(archive.read(name))
    return make_baseline(version, source_sha, index)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--repo", default="Plane-PPP/openyamm")
    args = parser.parse_args()
    if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", args.repo):
        parser.error("Invalid repository")
    report = json.loads(args.report.read_text())
    artifact_id = report["artifact_id"]
    metadata = json.loads(subprocess.run(
        ["gh", "api", f"repos/{args.repo}/actions/artifacts/{artifact_id}"],
        check=True, capture_output=True, text=True).stdout)
    if metadata["expired"] or metadata["workflow_run"]["head_sha"] != report["source_sha"]:
        raise ValueError("Baseline artifact expired or has a different source SHA")
    stream = RemoteZip(artifact_url(args.repo, artifact_id), metadata["size_in_bytes"])
    baseline = extract_baseline(stream, args.version, report["source_sha"])
    baseline["provenance"] = {"run": report["run"], "artifact_id": artifact_id,
                              "artifact_bytes": metadata["size_in_bytes"],
                              "metadata_bytes_downloaded": stream.downloaded}
    write_json(args.output, baseline)
    print(f"Baseline saved: {len(baseline['index']['packs'])} packs; fetched {stream.downloaded} bytes")


if __name__ == "__main__":
    main()
