from __future__ import annotations

import json
from http import HTTPStatus
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import re


ROOT_DIR = Path(__file__).resolve().parent
PRESET_DIR = ROOT_DIR / "animation-presets"
LEGACY_PRESET_FILE = ROOT_DIR / "animation-presets.json"
HOST = "127.0.0.1"
PORT = 8123


def ensure_preset_dir() -> None:
    PRESET_DIR.mkdir(exist_ok=True)


def list_preset_files() -> list[Path]:
    ensure_preset_dir()
    return sorted(
        [
            path
            for path in PRESET_DIR.iterdir()
            if path.is_file() and path.suffix.lower() == ".json"
        ]
    )


def load_presets() -> list:
    presets = []
    for path in list_preset_files():
        with path.open("r", encoding="utf-8") as file:
            preset = json.load(file)
        if not isinstance(preset, dict):
            raise ValueError(f"{path.name} 必须是对象")
        presets.append(preset)
    return presets


def validate_preset(payload: object) -> dict:
    if not isinstance(payload, dict):
        raise ValueError("请求体必须是对象")
    keys = set(payload.keys())
    if keys != {"name", "f", "d"}:
        raise ValueError("请求体字段必须且仅能包含 name、f 和 d")
    if not isinstance(payload["name"], str):
        raise ValueError("name 必须是字符串")
    if len(payload["name"].strip()) == 0:
        raise ValueError("name 不能为空")
    if not isinstance(payload["f"], int):
        raise ValueError("f 必须是整数")
    if not isinstance(payload["d"], list):
        raise ValueError("d 必须是数组")
    if len(payload["d"]) != payload["f"]:
        raise ValueError("f 必须等于 d 的长度")
    return {
        "name": payload["name"].strip(),
        "f": payload["f"],
        "d": payload["d"],
    }


def validate_delete_payload(payload: object) -> int:
    if not isinstance(payload, dict):
        raise ValueError("请求体必须是对象")
    keys = set(payload.keys())
    if keys != {"index"}:
        raise ValueError("请求体字段必须且仅能包含 index")
    if not isinstance(payload["index"], int):
        raise ValueError("index 必须是整数")
    return payload["index"]


def sanitize_filename_part(value: str) -> str:
    cleaned = re.sub(r"[^0-9A-Za-z\u4e00-\u9fff_-]+", "-", value).strip("-")
    if len(cleaned) == 0:
        return "preset"
    return cleaned[:40]


def get_next_preset_path(name: str) -> Path:
    files = list_preset_files()
    max_index = 0
    used_stems = set()
    for path in files:
        parts = path.stem.split("-", 1)
        if len(parts) == 0:
            continue
        if not parts[0].isdigit():
            continue
        current_index = int(parts[0])
        if current_index > max_index:
            max_index = current_index
        if len(parts) > 1:
            used_stems.add(parts[1])
    next_index = max_index + 1
    base_stem = sanitize_filename_part(name)
    candidate_stem = base_stem
    suffix = 2
    while candidate_stem in used_stems:
      candidate_stem = f"{base_stem}-{suffix}"
      suffix += 1
    filename = f"{next_index:06d}-{candidate_stem}.json"
    return PRESET_DIR / filename


def save_preset(preset: dict) -> None:
    path = get_next_preset_path(preset["name"])
    with path.open("w", encoding="utf-8") as file:
        json.dump(preset, file, ensure_ascii=False, indent=2)
        file.write("\n")


def delete_preset(index: int) -> int:
    files = list_preset_files()
    if index < 0 or index >= len(files):
        raise ValueError("index 超出范围")
    files[index].unlink()
    return len(files) - 1


def migrate_legacy_presets_if_needed() -> None:
    ensure_preset_dir()
    if len(list_preset_files()) > 0:
        return
    if not LEGACY_PRESET_FILE.exists():
        return

    with LEGACY_PRESET_FILE.open("r", encoding="utf-8") as file:
        data = json.load(file)

    if not isinstance(data, list):
        raise ValueError("animation-presets.json 必须是数组")

    for item in data:
        preset = validate_preset(item)
        save_preset(preset)

    with LEGACY_PRESET_FILE.open("w", encoding="utf-8") as file:
        json.dump([], file, ensure_ascii=False, indent=2)
        file.write("\n")


class PresetRequestHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(ROOT_DIR), **kwargs)

    def _send_json(self, status: HTTPStatus, payload: object) -> None:
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self) -> None:
        if self.path == "/api/animation-presets":
            try:
                presets = load_presets()
                self._send_json(HTTPStatus.OK, presets)
            except Exception as error:
                self._send_json(
                    HTTPStatus.INTERNAL_SERVER_ERROR,
                    {"error": str(error)},
                )
            return
        super().do_GET()

    def do_POST(self) -> None:
        if (
            self.path != "/api/animation-presets/append"
            and self.path != "/api/animation-presets/delete"
        ):
            self._send_json(HTTPStatus.NOT_FOUND, {"error": "接口不存在"})
            return

        content_length = self.headers.get("Content-Length")
        if content_length is None:
            self._send_json(HTTPStatus.BAD_REQUEST, {"error": "缺少 Content-Length"})
            return

        raw_body = self.rfile.read(int(content_length))
        try:
            payload = json.loads(raw_body.decode("utf-8"))
            if self.path == "/api/animation-presets/append":
                preset = validate_preset(payload)
                save_preset(preset)
                presets = load_presets()
                self._send_json(
                    HTTPStatus.OK,
                    {"index": len(presets) - 1, "count": len(presets)},
                )
                return

            index = validate_delete_payload(payload)
            count = delete_preset(index)
            self._send_json(
                HTTPStatus.OK,
                {"count": count},
            )
        except ValueError as error:
            self._send_json(HTTPStatus.BAD_REQUEST, {"error": str(error)})
        except Exception as error:
            self._send_json(
                HTTPStatus.INTERNAL_SERVER_ERROR,
                {"error": str(error)},
            )


def main() -> None:
    migrate_legacy_presets_if_needed()
    server = ThreadingHTTPServer((HOST, PORT), PresetRequestHandler)
    print(f"Preset server running at http://{HOST}:{PORT}/pixel-material-studio.html")
    server.serve_forever()


if __name__ == "__main__":
    main()
