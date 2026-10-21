#!/usr/bin/env python3
"""Turns a Snapper Studio study session into a short report.

    tools/study/report.py SESSION.jsonl [SESSION.jsonl ...]

Prints, per session: length, clicks and where they landed, pointer travel,
commands run, undo use, long pauses, and every note with its frustration
level. Notes split the session into tasks, so each task gets its own
numbers.
"""

import json
import math
import sys
from collections import Counter

PAUSE_MS = 2000


def read_events(path):
    with open(path, encoding="utf-8") as lines:
        return [json.loads(line) for line in lines if line.strip()]


def summarize(events):
    clicks = [e for e in events if e["type"] in ("click", "doubleclick")]
    moves = [e for e in events if e["type"] in ("move", "click")]
    travel = sum(
        math.dist((a["x"], a["y"]), (b["x"], b["y"]))
        for a, b in zip(moves, moves[1:])
    )
    pauses = sum(
        1 for a, b in zip(events, events[1:]) if b["t"] - a["t"] >= PAUSE_MS
    )
    steps = [e["index"] for e in events if e["type"] == "history"]
    undos = sum(1 for a, b in zip(steps, steps[1:]) if b < a)
    return {
        "seconds": (events[-1]["t"] - events[0]["t"]) / 1000 if events else 0,
        "clicks": len(clicks),
        "travel_px": round(travel),
        "pauses": pauses,
        "undos": undos,
        "commands": Counter(e["id"] for e in events if e["type"] == "command"),
        "controls": Counter(e.get("control", "?") for e in clicks),
    }


def print_summary(title, summary):
    print(f"  {title}: {summary['seconds']:.1f} s, {summary['clicks']} clicks, "
          f"{summary['travel_px']} px travel, {summary['pauses']} pauses, "
          f"{summary['undos']} undos")
    for control, count in summary["controls"].most_common(5):
        print(f"    clicked {control} x{count}")
    for command, count in summary["commands"].most_common(5):
        print(f"    ran {command} x{count}")


def report(path):
    events = read_events(path)
    print(path)
    print_summary("whole session", summarize(events))
    task = []
    number = 1
    for event in events:
        task.append(event)
        if event["type"] != "note":
            continue
        level = event["frustration"] or "not given"
        print(f"  note {number}: \"{event['text']}\" frustration {level}")
        print_summary(f"task {number}", summarize(task))
        task = []
        number += 1


def main():
    if len(sys.argv) < 2:
        print(__doc__.strip())
        return 2
    for path in sys.argv[1:]:
        report(path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
