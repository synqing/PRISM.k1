import unittest
from unittest import mock

from tools import check_deps


class CheckDepsTests(unittest.TestCase):
    @mock.patch("tools.check_deps.importlib.import_module")
    def test_missing_required_python_dependency_raises(self, mock_import):
        def side_effect(name):
            if name == "hsluv":
                raise ImportError("module not found")
            return object()

        mock_import.side_effect = side_effect
        with self.assertRaises(SystemExit) as exc:
            check_deps.check_python_modules()
        self.assertIn("Missing required Python dependency 'hsluv'", str(exc.exception))

    @mock.patch("tools.check_deps.shutil.which", return_value=None)
    def test_missing_optional_cli_emits_warning(self, mock_which):
        with mock.patch("sys.stderr") as stderr_mock:
            check_deps.check_cli_tools()
        mock_which.assert_called_with("aha")
        stderr_mock.write.assert_called()

    @mock.patch("tools.check_deps.importlib.import_module")
    @mock.patch("tools.check_deps.shutil.which", return_value="/usr/local/bin/aha")
    def test_main_success(self, mock_which, mock_import):
        check_deps.main()
        self.assertTrue(mock_import.called)
        mock_which.assert_called_once_with("aha")


if __name__ == "__main__":
    unittest.main()
