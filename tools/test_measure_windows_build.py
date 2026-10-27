import unittest
from measure_windows_build import GIB, choose_jobs, cpu_percent, low_memory


class ResourcePolicyTests(unittest.TestCase):
    def test_parallelism_follows_runner(self):
        self.assertEqual(choose_jobs(4, 13 * GIB), 4)
        self.assertEqual(choose_jobs(2, 7 * GIB), 2)
        self.assertEqual(choose_jobs(8, 22 * GIB), 8)
        self.assertEqual(choose_jobs(8, 8 * GIB), 3)
        with self.assertRaises(RuntimeError):
            choose_jobs(8, 3 * GIB)

    def test_pressure_checks_physical_and_commit(self):
        sample = dict(total_bytes=16 * GIB, available_bytes=8 * GIB, commit_available_bytes=4 * GIB)
        self.assertFalse(low_memory(sample))
        self.assertTrue(low_memory(dict(sample, available_bytes=GIB)))
        self.assertTrue(low_memory(dict(sample, commit_available_bytes=0)))

    def test_cpu_is_runner_percent(self):
        self.assertEqual(cpu_percent(dict(cpu_total_ticks=0, cpu_idle_ticks=0),
                                     dict(cpu_total_ticks=400, cpu_idle_ticks=100)), 75)


if __name__ == '__main__':
    unittest.main()
