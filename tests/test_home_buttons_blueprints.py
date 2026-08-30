"""Structural tests for the shipped Home Assistant automation blueprints.

Loads the blueprint YAML from the repo (not a copy) and checks it against the
firmware MQTT discovery contract in mqtt_helper.cpp / hardware.cpp / config.h.
"""

from __future__ import annotations

import re
import unittest
from pathlib import Path

import yaml

REPO_ROOT = Path(__file__).resolve().parents[1]

FIRMWARE_CONFIG = REPO_ROOT / "Firmware/HomeButtonsArduino/src/config.h"
FIRMWARE_HARDWARE = REPO_ROOT / "Firmware/HomeButtonsArduino/src/hardware.cpp"
FIRMWARE_MQTT_HELPER = REPO_ROOT / "Firmware/HomeButtonsArduino/src/mqtt_helper.cpp"

BLUEPRINT_CASES = (
    {
        "path": REPO_ROOT / "blueprints/automation/home_buttons.yaml",
        "firmware_symbol": "HOME_BUTTONS_ORIGINAL",
        "model": "Home Buttons",
    },
    {
        "path": REPO_ROOT / "blueprints/automation/home_buttons_mini.yaml",
        "firmware_symbol": "HOME_BUTTONS_MINI",
        "model": "Home Buttons Mini",
    },
)


class InputRef:
    """Value of a Home Assistant ``!input`` YAML tag."""

    def __init__(self, name: str) -> None:
        self.name = name

    def __repr__(self) -> str:
        return f"!input {self.name}"

    def __eq__(self, other: object) -> bool:
        return isinstance(other, InputRef) and self.name == other.name


class HALoader(yaml.SafeLoader):
    pass


def _construct_input(loader: yaml.Loader, node: yaml.Node) -> InputRef:
    return InputRef(loader.construct_scalar(node))


HALoader.add_constructor("!input", _construct_input)


def load_blueprint(path: Path) -> dict:
    with path.open(encoding="utf-8") as handle:
        doc = yaml.load(handle, Loader=HALoader)
    if not isinstance(doc, dict):
        raise TypeError(f"{path} did not parse as a mapping")
    return doc


def firmware_manufacturer() -> str:
    text = FIRMWARE_CONFIG.read_text(encoding="utf-8")
    match = re.search(
        r'static constexpr char MANUFACTURER\[\] = "([^"]+)";', text
    )
    if not match:
        raise AssertionError("MANUFACTURER not found in config.h")
    return match.group(1)


def firmware_num_buttons(symbol: str) -> int:
    text = FIRMWARE_CONFIG.read_text(encoding="utf-8")
    match = re.search(
        rf"(?:#if|#elif) defined\({re.escape(symbol)}\)\s*"
        r"static constexpr uint8_t NUM_BUTTONS = (\d+);",
        text,
    )
    if not match:
        raise AssertionError(f"NUM_BUTTONS for {symbol} not found in config.h")
    return int(match.group(1))


def firmware_model_names() -> set[str]:
    text = FIRMWARE_HARDWARE.read_text(encoding="utf-8")
    names = set(
        re.findall(r'strncpy\(model_name_, "([^"]+)", sizeof\(model_name_\)\)', text)
    )
    if not names:
        raise AssertionError("model names not found in hardware.cpp")
    return names


def firmware_press_types() -> tuple[str, ...]:
    text = FIRMWARE_MQTT_HELPER.read_text(encoding="utf-8")
    types = tuple(re.findall(r'conf\["type"\] = "([^"]+)";', text))
    if not types:
        raise AssertionError("no conf[\"type\"] assignments in mqtt_helper.cpp")
    return types


def firmware_subtype_template() -> str:
    text = FIRMWARE_MQTT_HELPER.read_text(encoding="utf-8")
    match = re.search(
        r'conf\["stype"\] = FormatterType\("([^"]+)", bsl\.id\(\)\)', text
    )
    if not match:
        raise AssertionError("stype FormatterType not found in mqtt_helper.cpp")
    return match.group(1)


def flatten_inputs(inputs: dict) -> dict:
    """Flatten blueprint input sections into a name -> spec mapping."""
    flat: dict = {}
    for key, spec in inputs.items():
        if (
            isinstance(spec, dict)
            and "input" in spec
            and "selector" not in spec
        ):
            nested = spec["input"]
            if not isinstance(nested, dict):
                raise TypeError(f"section {key!r} input is not a mapping")
            overlap = set(flat) & set(nested)
            if overlap:
                raise AssertionError(f"duplicate input names in section {key}: {overlap}")
            flat.update(flatten_inputs(nested))
        else:
            if key in flat:
                raise AssertionError(f"duplicate input name {key!r}")
            flat[key] = spec
    return flat


def as_list(value):
    if value is None:
        return []
    if isinstance(value, list):
        return value
    return [value]


def document_triggers(doc: dict) -> list[dict]:
    triggers = doc.get("triggers", doc.get("trigger"))
    return [t for t in as_list(triggers) if isinstance(t, dict)]


def document_actions(doc: dict) -> list:
    return as_list(doc.get("actions", doc.get("action")))


def is_mqtt_device_trigger(trigger: dict) -> bool:
    is_device = trigger.get("platform") == "device" or trigger.get("trigger") == "device"
    return is_device and trigger.get("domain") == "mqtt"


def trigger_ids_from_conditions(conditions) -> list:
    ids: list = []
    for cond in as_list(conditions):
        if not isinstance(cond, dict):
            continue
        if cond.get("condition") != "trigger":
            continue
        tid = cond.get("id")
        if isinstance(tid, list):
            ids.extend(tid)
        elif tid is not None:
            ids.append(tid)
    return ids


def invoked_input_name(sequence) -> str | None:
    if isinstance(sequence, InputRef):
        return sequence.name
    if isinstance(sequence, list):
        refs = [item.name for item in sequence if isinstance(item, InputRef)]
        if len(refs) == 1:
            return refs[0]
    return None


def collect_trigger_routes(actions) -> dict[object, list[str]]:
    """Map trigger id -> list of action-input names invoked by choose/if branches."""
    routes: dict[object, list[str]] = {}

    def add(tid, sequence) -> None:
        name = invoked_input_name(sequence)
        if name is None:
            return
        routes.setdefault(tid, []).append(name)

    def walk(node) -> None:
        for action in as_list(node):
            if not isinstance(action, dict):
                continue
            if "choose" in action:
                for branch in as_list(action.get("choose")):
                    if not isinstance(branch, dict):
                        continue
                    for tid in trigger_ids_from_conditions(branch.get("conditions")):
                        add(tid, branch.get("sequence"))
            if "if" in action:
                for tid in trigger_ids_from_conditions(action.get("if")):
                    add(tid, action.get("then"))
            for key in ("sequence", "then", "else", "default"):
                if key in action:
                    walk(action[key])

    walk(actions)
    return routes


def device_selector_filters(device_spec: dict) -> list[dict]:
    selector = device_spec.get("selector") or {}
    device = selector.get("device")
    if not isinstance(device, dict):
        return []
    filters = device.get("filter", [])
    if isinstance(filters, dict):
        return [filters]
    return [f for f in filters if isinstance(f, dict)]


def find_device_input(flat_inputs: dict) -> tuple[str, dict]:
    found = [
        (name, spec)
        for name, spec in flat_inputs.items()
        if isinstance(spec, dict)
        and isinstance(spec.get("selector"), dict)
        and "device" in spec["selector"]
    ]
    if len(found) != 1:
        raise AssertionError(f"expected exactly one device input, found {len(found)}")
    return found[0]


class BlueprintContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.manufacturer = firmware_manufacturer()
        cls.press_types = firmware_press_types()
        cls.subtype_template = firmware_subtype_template()
        cls.model_names = firmware_model_names()

    def test_firmware_contract_is_the_expected_discovery_shape(self) -> None:
        self.assertEqual(self.manufacturer, "PLab")
        self.assertEqual(
            self.press_types,
            (
                "button_short_press",
                "button_double_press",
                "button_triple_press",
                "button_quadruple_press",
            ),
        )
        self.assertEqual(self.subtype_template, "button_%d")
        self.assertIn("Home Buttons", self.model_names)
        self.assertIn("Home Buttons Mini", self.model_names)

    def test_each_shipped_blueprint_matches_firmware_cartesian(self) -> None:
        for case in BLUEPRINT_CASES:
            with self.subTest(blueprint=str(case["path"].relative_to(REPO_ROOT))):
                self._assert_blueprint(case)

    def _assert_blueprint(self, case: dict) -> None:
        path: Path = case["path"]
        self.assertTrue(path.is_file(), f"missing shipped blueprint {path}")

        num_buttons = firmware_num_buttons(case["firmware_symbol"])
        model = case["model"]
        self.assertIn(model, self.model_names)

        doc = load_blueprint(path)
        meta = doc.get("blueprint")
        self.assertIsInstance(meta, dict)
        self.assertEqual(meta.get("domain"), "automation")

        min_version = (meta.get("homeassistant") or {}).get("min_version", "0.0.0")
        self.assertGreaterEqual(
            tuple(int(p) for p in str(min_version).split(".")[:3]),
            (2024, 2, 0),
            "MQTT device triggers without discovery_id require HA >= 2024.2.0",
        )

        raw_inputs = meta.get("input")
        self.assertIsInstance(raw_inputs, dict)
        flat_inputs = flatten_inputs(raw_inputs)
        device_input_name, device_spec = find_device_input(flat_inputs)

        filters = device_selector_filters(device_spec)
        self.assertTrue(
            any(
                filt.get("integration") == "mqtt"
                and filt.get("manufacturer") == self.manufacturer
                and filt.get("model") == model
                for filt in filters
            ),
            f"device selector filter must be mqtt + {self.manufacturer} + {model!r}, got {filters}",
        )

        triggers = document_triggers(doc)
        mqtt_triggers = [t for t in triggers if is_mqtt_device_trigger(t)]
        self.assertTrue(mqtt_triggers, "blueprint has no MQTT device triggers")

        pair_to_trigger: dict[tuple[int, str], dict] = {}
        for trigger in mqtt_triggers:
            self.assertIsInstance(trigger.get("device_id"), InputRef)
            self.assertEqual(trigger["device_id"].name, device_input_name)
            self.assertNotIn(
                "discovery_id",
                trigger,
                "omit discovery_id; HA >= 2024.2 matches device + type + subtype",
            )
            key = (trigger.get("subtype"), trigger.get("type"))
            self.assertNotIn(key, pair_to_trigger, f"duplicate trigger {key}")
            pair_to_trigger[key] = trigger

        routes = collect_trigger_routes(document_actions(doc))
        used_action_inputs: set[str] = set()
        expected_pairs = [
            (self.subtype_template % button, press_type)
            for button in range(1, num_buttons + 1)
            for press_type in self.press_types
        ]

        for subtype, press_type in expected_pairs:
            with self.subTest(subtype=subtype, type=press_type):
                trigger = pair_to_trigger.get((subtype, press_type))
                self.assertIsNotNone(
                    trigger,
                    f"missing MQTT device trigger type={press_type} subtype={subtype}",
                )
                trigger_id = trigger["id"]
                self.assertTrue(trigger_id, "trigger must have an id for routing")
                invoked = routes.get(trigger_id)
                self.assertIsNotNone(
                    invoked, f"no choose/if branch for trigger id {trigger_id!r}"
                )
                self.assertEqual(
                    len(invoked),
                    1,
                    f"trigger id {trigger_id!r} must route to exactly one action input, got {invoked}",
                )
                action_name = invoked[0]
                spec = flat_inputs.get(action_name)
                self.assertIsInstance(
                    spec, dict, f"action input {action_name!r} is not defined"
                )
                self.assertEqual(
                    spec.get("default"),
                    [],
                    f"action input {action_name!r} must default to []",
                )
                selector = spec.get("selector")
                self.assertIsInstance(selector, dict)
                self.assertIn(
                    "action",
                    selector,
                    f"input {action_name!r} must use an action selector",
                )
                self.assertNotIn(action_name, used_action_inputs)
                used_action_inputs.add(action_name)

        self.assertEqual(
            len(expected_pairs),
            len(used_action_inputs),
            f"expected {len(expected_pairs)} unique action inputs, got {len(used_action_inputs)}",
        )
        extra = {
            (subtype, press_type)
            for subtype, press_type in pair_to_trigger
            if (subtype, press_type) not in {(s, t) for s, t in expected_pairs}
        }
        self.assertFalse(extra, f"unexpected MQTT device triggers: {extra}")
        print(
            f"{path.relative_to(REPO_ROOT)}: covered {len(expected_pairs)} "
            f"button×press pairs ({num_buttons}×{len(self.press_types)})"
        )


if __name__ == "__main__":
    unittest.main()
