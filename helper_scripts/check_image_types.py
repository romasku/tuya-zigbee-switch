"""Check device_db.yaml for duplicate firmware_image_type values.

Each device must own a unique firmware_image_type. OTA clients (zigpy / Home
Assistant ZHA and Zigbee2MQTT) normally disambiguate a shared imageType by
manufacturerName, so a duplicate is usually masked -- but it breaks OTA outright
when two colliding devices also share a manufacturer name, and an entry left
without a manufacturerName matches the wrong device. A unique id per device (as
the db intends) avoids relying on that fragile fallback.

Modes:
  --changed BASE_REF
      Only devices whose firmware_image_type is new or changed versus BASE_REF
      are checked. Pre-existing duplicates do not fail, so this never turns red
      on unrelated PRs; but no PR can claim an image type that is already taken.
      Use on pull requests.

  --all
      Every device is checked; fails if ANY firmware_image_type is shared.
      Use on the main branch to assert the whole db stays clean.
"""
import argparse
import subprocess
import sys

import yaml

DB = "device_db.yaml"


def image_types(db):
    result = {}
    for name, fields in (db or {}).items():
        if not isinstance(fields, dict):
            continue
        image_type = fields.get("firmware_image_type")
        if image_type in (None, "null"):
            continue
        result[name] = image_type
    return result


def load_worktree():
    with open(DB) as f:
        return yaml.safe_load(f)


def load_ref(ref):
    blob = subprocess.run(
        ["git", "show", f"{ref}:{DB}"], capture_output=True, text=True
    )
    if blob.returncode != 0:
        print(f"warning: cannot read {DB} from {ref}, treating base as empty")
        return {}
    return yaml.safe_load(blob.stdout)


def owners_by_type(head):
    owners = {}
    for name, image_type in head.items():
        owners.setdefault(image_type, []).append(name)
    return owners


def fail(collisions):
    print("firmware_image_type collisions:\n")
    for image_type, devices in sorted(collisions.items()):
        print(f"  {image_type}: {', '.join(sorted(devices))}")
    print("\nEach device needs a unique firmware_image_type.")
    print("Get the next free id with: make tools/unused_image_type")
    sys.exit(1)


def check_all(head):
    owners = owners_by_type(head)
    collisions = {it: devs for it, devs in owners.items() if len(devs) > 1}
    if collisions:
        fail(collisions)
    print(f"OK: {len(head)} devices, all firmware_image_type values unique")


def check_changed(head, base_ref):
    base = image_types(load_ref(base_ref))
    owners = owners_by_type(head)
    changed = [name for name, it in head.items() if base.get(name) != it]
    collisions = {}
    for name in changed:
        others = [o for o in owners[head[name]] if o != name]
        if others:
            collisions.setdefault(head[name], set()).update([name, *others])
    if collisions:
        fail(collisions)
    print(f"OK: {len(changed)} added/changed entries checked against "
          f"{base_ref}, no image_type collisions")


def main():
    parser = argparse.ArgumentParser(
        description="Check firmware_image_type uniqueness in device_db.yaml")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--all", action="store_true",
                       help="check every device (use on main)")
    group.add_argument("--changed", metavar="BASE_REF",
                       help="check only entries changed vs BASE_REF (use on PRs)")
    args = parser.parse_args()

    head = image_types(load_worktree())
    if args.all:
        check_all(head)
    else:
        check_changed(head, args.changed)


if __name__ == "__main__":
    main()
