from __future__ import annotations

from pathlib import Path

from tools.runtime_bundle import generate_bundle_header, stage_runtime_bundle


def test_generate_bundle_header_contains_auto_start_and_files(tmp_path: Path):
    bundle_dir = tmp_path / "bundle"
    (bundle_dir / "maixapp" / "apps" / "demo").mkdir(parents=True)
    (bundle_dir / "maixapp" / "auto_start.txt").write_text("demo\n", encoding="utf-8")
    (bundle_dir / "maixapp" / "apps" / "demo" / "main.py").write_text("print('demo')\n", encoding="utf-8")
    (bundle_dir / "models").mkdir(parents=True)
    (bundle_dir / "models" / "model.tflite").write_bytes(b"\x20\x00\x00\x00TFL3")

    output = tmp_path / "bundle.h"
    generate_bundle_header(bundle_dir, output)
    text = output.read_text(encoding="utf-8")

    assert "MAIX_RUNTIME_BUNDLE_AUTO_START_APP_ID" in text
    assert '"/maixapp/apps/demo/main.py"' in text
    assert '"/models/model.tflite"' in text
    assert "0x54, 0x46, 0x4c, 0x33" in text


def test_stage_runtime_bundle_copies_app_and_model(tmp_path: Path):
    app_dir = tmp_path / "app"
    app_dir.mkdir()
    (app_dir / "main.py").write_text("print('app')\n", encoding="utf-8")
    (app_dir / "helper.py").write_text("VALUE = 1\n", encoding="utf-8")
    model = tmp_path / "model.kmodel"
    model.write_bytes(b"KMODEL")
    bundle_dir = tmp_path / "bundle"

    stage_runtime_bundle(
        app_dir=app_dir,
        bundle_dir=bundle_dir,
        model_path=model,
        app_id="demo_app",
        reset_app=True,
    )

    assert (bundle_dir / "maixapp" / "auto_start.txt").read_text(encoding="utf-8").strip() == "demo_app"
    assert (bundle_dir / "maixapp" / "apps" / "demo_app" / "main.py").exists()
    assert (bundle_dir / "maixapp" / "apps" / "demo_app" / "helper.py").exists()
    assert (bundle_dir / "maixapp" / "apps" / "demo_app" / "app.yaml").exists()
    assert (bundle_dir / "models" / "model.kmodel").read_bytes() == b"KMODEL"


def test_stage_runtime_bundle_generates_manifest_and_copies_labels(tmp_path: Path):
    app_dir = tmp_path / "app"
    app_dir.mkdir()
    (app_dir / "main.py").write_text("print('app')\n", encoding="utf-8")
    model = tmp_path / "demo.tflite"
    model.write_bytes(b"\x20\x00\x00\x00TFL3")
    labels = tmp_path / "labels.txt"
    labels.write_text("cat\ndog\n", encoding="utf-8")
    bundle_dir = tmp_path / "bundle"

    stage_runtime_bundle(
        app_dir=app_dir,
        bundle_dir=bundle_dir,
        model_path=model,
        labels_path=labels,
        app_id="demo_app",
    )

    manifest = (bundle_dir / "models" / "model.manifest").read_text(encoding="utf-8")
    assert "task=classification" in manifest
    assert "default=demo.tflite" in manifest
    assert "tflite=demo.tflite" in manifest
    assert "labels=labels.txt" in manifest
    assert (bundle_dir / "models" / "labels.txt").read_text(encoding="utf-8") == "cat\ndog\n"
