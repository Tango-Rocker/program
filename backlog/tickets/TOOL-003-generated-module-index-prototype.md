# TICKET ID: TOOL-003-generated-module-index-prototype

## Objective

Add a deterministic generated module index prototype.

## Context

Known issues list generated indexes as missing. This ticket adds a small tool-owned generated artifact so agents can orient without manually maintaining derived maps.

## Sprint

Sprint 4 - replay, indexes, data boundary, and debug surface.

## Acceptance criteria

- Indexer scans documented module directories and emits stable sorted output.
- Generated output includes source directory, public headers if present, tests if present, and owning docs.
- Generated file is produced by a tool command and is not manually edited.
- Scripts exist for Unix shell and PowerShell or the project documents why only one is supported.
- Running the generator twice without source changes produces no diff.

## Allowed files

- `tools/indexers/module_index.py`
- `scripts/generate_indexes.sh`
- `scripts/generate-indexes.ps1`
- `generated/module_index.md`
- `docs/00_INDEX.md`
- `docs/15_AGENT_WORKFLOW.md`

## Out of scope

- Full dependency graph extraction.
- LSP integration.
- Build system generation.
- Editing other generated files manually.

## Required checks

- Run the index generator.
- Run the index generator a second time and confirm output is stable.
- Do not manually edit files under `generated/`.

## Notes

If Python is used, keep the script dependency-free unless a future ticket explicitly introduces tool dependencies.
