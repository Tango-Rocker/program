#!/usr/bin/env python3

from __future__ import annotations

import pathlib


ROOT = pathlib.Path(__file__).resolve().parents[2]
SRC_DIR = ROOT / "src"
TEST_DIR = ROOT / "tests"
DOC_DIR = ROOT / "docs"
GEN_DIR = ROOT / "generated"
OUT_PATH = GEN_DIR / "module_index.md"


MODULE_DOC_MAP = {
    "core": ["docs/04_CODING_STANDARD.md", "docs/00_INDEX.md"],
    "ecs": ["docs/02_ARCHITECTURE_MAP.md", "docs/06_ECS_AND_EVENTS.md"],
    "event": ["docs/06_ECS_AND_EVENTS.md"],
    "world": ["docs/07_NAVIGATION.md", "docs/03_MODULE_MAP.md", "docs/12_TESTING.md"],
    "nav": ["docs/07_NAVIGATION.md", "docs/00_INDEX.md"],
    "sim": ["docs/06_ECS_AND_EVENTS.md", "docs/12_TESTING.md", "docs/14_SIMULATION_DOCTRINE.md"],
    "render": ["docs/08_UI_UX.md", "docs/12_TESTING.md"],
    "audio": ["docs/11_PROJECTILES_BUFFS_PARTICLES.md"],
    "colony": ["docs/09_COLONY_JOBS.md"],
    "platform": ["docs/08_UI_UX.md", "docs/15_AGENT_WORKFLOW.md"],
    "lua": ["docs/05_DATA_AND_LUA.md", "docs/12_TESTING.md"],
    "ui": ["docs/08_UI_UX.md", "docs/12_TESTING.md"],
    "game": ["README.md"],
}

MODULE_TEST_MAP = {
    "core": ["test_arena.c", "test_random.c"],
    "ecs": ["test_entity_registry.c"],
    "event": ["test_event_queue.c", "test_event_log.c"],
    "world": ["test_hex.c", "test_tile_field.c", "test_world_map.c", "test_structure.c", "test_topology.c", "test_scenario_gen.c"],
    "nav": ["test_pathfind.c", "test_path_service.c"],
    "sim": [
        "test_sim_tick.c",
        "test_command_queue.c",
        "test_noise_field.c",
        "test_replay.c",
        "test_replay_golden.c",
        "test_scheduler.c",
        "test_snapshot.c",
        "test_sensory_fields.c",
        "test_horde_attention.c",
        "test_horde_group.c",
        "test_horde_materialization.c",
        "test_party.c",
        "test_combat_ability.c",
        "test_projectile.c",
        "test_status_effect.c",
        "test_particle_request.c",
    ],
    "render": ["test_camera.c", "test_particle_request.c"],
    "audio": ["test_audio_request.c"],
    "colony": [
        "test_job_board.c",
        "test_worker_jobs.c",
        "test_inventory.c",
        "test_haul_job.c",
        "test_construction.c",
        "test_colony_emergency.c",
    ],
    "platform": [],
    "lua": ["test_lua_host.c", "test_content_schema.c"],
    "ui": ["test_command_inspector.c", "test_field_overlay.c", "test_causal_report.c"],
    "game": ["test_default_scene.c"],
}


def _to_posix(path: pathlib.Path) -> str:
    return path.as_posix()


def _sorted_relative(paths):
    return sorted(_to_posix(p.relative_to(ROOT)) for p in paths)


def _collect_sources(module_dir: pathlib.Path):
    sources = []
    headers = []
    for path in sorted(module_dir.rglob("*")):
        if not path.is_file():
            continue
        if path.suffix.lower() == ".c":
            sources.append(path)
        elif path.suffix.lower() == ".h":
            headers.append(path)
    return _sorted_relative(sources), _sorted_relative(headers)


def _collect_tests(module_name: str):
    return sorted(MODULE_TEST_MAP.get(module_name, []))


def _collect_docs(module_name: str):
    return sorted(MODULE_DOC_MAP.get(module_name, []))


def _build_module_sections():
    lines = ["# Module Index\n", "\n", "This index is generated and stable.\n", "\n", "## Modules\n", "\n"]
    for module_dir in sorted(SRC_DIR.iterdir(), key=lambda p: p.name):
        if not module_dir.is_dir():
            continue
        name = module_dir.name
        sources, headers = _collect_sources(module_dir)

        lines.append(f"- `{_to_posix(module_dir.relative_to(ROOT))}/`\n")
        lines.append(f"  - Sources:\n")
        if sources:
            for source in sources:
                lines.append(f"    - `{source}`\n")
        else:
            lines.append("    - (none)\n")

        lines.append(f"  - Public headers:\n")
        if headers:
            for header in headers:
                lines.append(f"    - `{header}`\n")
        else:
            lines.append("    - (none)\n")

        lines.append("  - Tests:\n")
        tests = _collect_tests(name)
        if tests:
            for test_name in tests:
                lines.append(f"    - `tests/{test_name}`\n")
        else:
            lines.append("    - (none)\n")

        lines.append("  - Owning docs:\n")
        docs = _collect_docs(name)
        if docs:
            for doc in docs:
                lines.append(f"    - `{doc}`\n")
        else:
            lines.append("    - (none)\n")

        lines.append("\n")

    return "".join(lines)


def main() -> int:
    GEN_DIR.mkdir(parents=True, exist_ok=True)
    lines = _build_module_sections()
    OUT_PATH.write_text(lines, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
