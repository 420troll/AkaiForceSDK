import importlib.util
import pathlib
import unittest

SCRIPT = pathlib.Path(__file__).resolve().parents[2] / 'tools/audio_validation/run.py'
spec = importlib.util.spec_from_file_location('es5510_audio_validation', SCRIPT)
validation = importlib.util.module_from_spec(spec)
spec.loader.exec_module(validation)


class AudioValidationTests(unittest.TestCase):
    def test_five_stimuli_are_deterministic_and_stereo(self):
        first = validation.stimuli(44100, 1024)
        second = validation.stimuli(44100, 1024)
        self.assertEqual(first, second)
        self.assertEqual(set(first), {'impulse', 'sine', 'sweep', 'noise', 'stereo_phase'})
        self.assertEqual(first['impulse'][128], (24000, 0))
        self.assertTrue(any(left != right for left, right in first['stereo_phase']))

    def test_metrics_locate_frame_divergence_and_alignment(self):
        reference = [[0, 0], [1, -1], [2, -2], [0, 0]]
        candidate = [[0, 0], [0, 0], [1, -1], [2, -2]]
        result = validation.metrics(reference, candidate)
        self.assertFalse(result['bit_exact'])
        self.assertEqual(result['best_alignment_lag_frames'], 1)
        self.assertEqual(result['first_divergent_frame'], 1)
        self.assertEqual(result['max_abs_deviation'], 2)

    def test_exact_comparison_reports_no_divergence(self):
        samples = [[-1200, 2300], [0, 0], [32000, -31000]]
        result = validation.metrics(samples, [row[:] for row in samples])
        self.assertTrue(result['bit_exact'])
        self.assertIsNone(result['first_divergent_frame'])
        self.assertEqual(result['max_abs_deviation'], 0)
        self.assertEqual(result['rms_deviation'], 0.0)
        self.assertEqual(result['best_alignment_lag_frames'], 0)


if __name__ == '__main__':
    unittest.main()
