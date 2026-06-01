#!/usr/bin/env python3

from __future__ import annotations

import pathlib
import re


ROOT = pathlib.Path(__file__).resolve().parents[2]
BACKLOG = ROOT / "backlog"
TICKETS = BACKLOG / "tickets"
GENERATED = ROOT / "generated"
OUT_PATH = GENERATED / "sprint_status.md"

TICKET_RE = re.compile(r"\b[A-Z]+-\d{3}-[a-z0-9-]+\b")
TICKET_ID_RE = re.compile(r"\b[A-Z]+-\d{3}\b")
SPRINT_RE = re.compile(r"^## Sprint (\d+) - (.+)$")


def read_ticket_ids(path: pathlib.Path) -> list[str]:
    ids: list[str] = []
    if not path.exists():
        return ids
    for line in path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if not stripped.startswith("- "):
            continue
        match = TICKET_ID_RE.search(stripped)
        if match:
            ids.append(match.group(0))
    return ids


def ticket_id_from_file(path: pathlib.Path) -> str:
    match = TICKET_ID_RE.match(path.name)
    return match.group(0) if match else path.stem


def collect_ticket_files() -> dict[str, pathlib.Path]:
    files: dict[str, pathlib.Path] = {}
    for path in sorted(TICKETS.glob("*.md"), key=lambda p: p.name):
        files[ticket_id_from_file(path)] = path
    return files


def parse_sprints() -> list[tuple[str, str, list[str]]]:
    sprint_path = BACKLOG / "sprints.md"
    sprints: list[tuple[str, str, list[str]]] = []
    current_number = ""
    current_title = ""
    in_tickets = False
    current_tickets: list[str] = []

    for line in sprint_path.read_text(encoding="utf-8").splitlines():
        sprint_match = SPRINT_RE.match(line)
        if sprint_match:
            if current_number:
                sprints.append((current_number, current_title, current_tickets))
            current_number = sprint_match.group(1)
            current_title = sprint_match.group(2)
            current_tickets = []
            in_tickets = False
            continue
        if not current_number:
            continue
        if line.strip() == "Tickets:":
            in_tickets = True
            continue
        if in_tickets and line.startswith("## "):
            in_tickets = False
        if in_tickets:
            match = TICKET_ID_RE.search(line)
            if match:
                current_tickets.append(match.group(0))

    if current_number:
        sprints.append((current_number, current_title, current_tickets))
    return sprints


def status_for(ticket_id: str, todo: set[str], active: set[str], done: set[str], files: dict[str, pathlib.Path]) -> str:
    status_count = int(ticket_id in todo) + int(ticket_id in active) + int(ticket_id in done)
    if status_count > 1:
        return "duplicate"
    if ticket_id not in files:
        return "missing"
    if ticket_id in done:
        return "done"
    if ticket_id in active:
        return "active"
    if ticket_id in todo:
        return "todo"
    return "missing"


def build_report() -> str:
    todo = set(read_ticket_ids(BACKLOG / "todo.md"))
    done = set(read_ticket_ids(BACKLOG / "done.md"))
    active = set(read_ticket_ids(BACKLOG / "active.md"))
    files = collect_ticket_files()
    listed = set(todo) | set(done) | set(active)

    lines = [
        "# Sprint Status\n",
        "\n",
        "This report is generated from backlog files. Do not edit it by hand.\n",
        "\n",
    ]

    seen: set[str] = set()
    for sprint_number, sprint_title, ticket_ids in parse_sprints():
        lines.append(f"## Sprint {sprint_number} - {sprint_title}\n\n")
        if not ticket_ids:
            lines.append("- (no tickets listed)\n\n")
            continue
        for ticket_id in ticket_ids:
            seen.add(ticket_id)
            status = status_for(ticket_id, todo, active, done, files)
            file_note = files.get(ticket_id)
            suffix = f" (`{file_note.relative_to(ROOT).as_posix()}`)" if file_note else ""
            lines.append(f"- `{ticket_id}`: {status}{suffix}\n")
        lines.append("\n")

    orphan_files = sorted(set(files) - seen)
    if orphan_files:
        lines.append("## Ticket Files Not Listed In Sprints\n\n")
        for ticket_id in orphan_files:
            status = status_for(ticket_id, todo, active, done, files)
            lines.append(f"- `{ticket_id}`: {status} (`{files[ticket_id].relative_to(ROOT).as_posix()}`)\n")
        lines.append("\n")

    listed_without_file = sorted(ticket_id for ticket_id in listed if ticket_id not in files)
    if listed_without_file:
        lines.append("## Backlog Entries Without Ticket Files\n\n")
        for ticket_id in listed_without_file:
            lines.append(f"- `{ticket_id}`: missing\n")
        lines.append("\n")

    return "".join(lines)


def main() -> int:
    GENERATED.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(build_report(), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
