

def test_command_start_finish_marker_count(setup_home_and_cleanup):
    """Repeated execution should emit one START and one FINISH marker per run."""
    home = setup_home_and_cleanup
    with tempfile.TemporaryDirectory() as tmp_dir:
        marker_cmd = "printf 'START\\n'; printf 'FINISH\\n'"
        run_pk("-a", input_text=f"{tmp_dir}\n{marker_cmd}\n", cwd=tmp_dir)

        run_count = 10
        for _ in range(run_count):
            result = run_pk("-e", "1.1", input_text="Y\n", cwd=tmp_dir)
            assert result.returncode == 0, f"stderr: {result.stderr}"
            assert result.stdout.count("START") == 1, result.stdout
            assert result.stdout.count("FINISH") == 1, result.stdout

        config = read_config(home)
        assert tmp_dir in config["path"]
        assert len(config["path"][tmp_dir]) == 1
