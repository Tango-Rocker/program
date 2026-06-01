# 09_COLONY_JOBS

## Work orders and jobs

Workers submit and consume work orders from a shared board.

## Core concepts

- Work orders define required actor, resource cost, and completion condition.
- Jobs are concrete assignments derived from work orders.
- Reservations prevent duplicate conflict for the same target.
- Workers have roles and stamina-aware schedules.
- Threat states alter job availability and urgency.

## Planned behavior

- colony AI should keep operating while player explores.
- emergency work orders interrupt routine labor.
- logs should preserve why a worker chose a job order.

## Job board contract

- Board entries expose:
  - immutable work id
  - target actor/sector
  - required resources and duration band
  - reservation owner (`none` or worker id)
  - state: `OPEN`, `RESERVED`, `IN_PROGRESS`, `STALLED`, `DONE`, `ABORTED`.
- Reservation policy:
  - reserved jobs lock target and prevent duplicate claim in deterministic lock-step.
  - stale reservations auto-expire after a deterministic timeout and return to `OPEN`.
- Threat response:
  - workers with active threat state switch to `STALLED` or `EMERGENCY_ASSIST`.
  - emergency orders can preempt `IN_PROGRESS` jobs with explicit audit log entries.

## Current implementation notes

- `src/colony/job_board.[ch]` provides deterministic fixed-capacity storage and transitions.
- Work order creation records:
  - `stable_id`, target axial position, required role/resource flags, and duration band.
- Reservation API tracks exclusive claimant with deterministic timeout via `game_job_board_advance_tick`.
- State machine currently supports `OPEN`, `RESERVED`, `IN_PROGRESS`, `STALLED`, `DONE`, `ABORTED`.
- `src/colony/worker_ai.[ch]` adds deterministic worker selection over open/routinely reserved jobs.
- Worker selection ranks candidates by role match, `duration_min_ticks` urgency, distance from worker position, then stable id.
- `GAME_WORKER_ROLE_EMERGENCY` jobs are preferred in threat mode while non-emergency routine jobs are stalled.
- Each decision emits `GameWorkerSelectionAuditEntry` so headless tests can verify reason codes (selected, stalled, avoided reservation, etc.).
- Construction and threat interruptions now add dedicated colony systems:
  - `src/colony/construction.[ch]` models deterministic worksite progression from resource haul through build completion.
  - Worksite creation creates a parent board order and per-requirement haul jobs with stable slot handles.
  - Completion places world structures, emits construction audit entries, and writes blocking footprint values into map tiles.
  - `src/colony/emergency.[ch]` stalls routine jobs on threat, creates emergency replacement work via board entries,
    and supports deterministic resume or abort when threat clears.
  - `src/colony/construction.[ch]` and `src/colony/emergency.[ch]` both emit explicit audit codes for replay and forensic tracing.
