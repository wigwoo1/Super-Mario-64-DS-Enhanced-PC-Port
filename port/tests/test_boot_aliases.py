"""Regress the namespace/extern-C brace pattern that aliased thiscall members."""
import importlib.util
import pathlib
import unittest

spec = importlib.util.spec_from_file_location("boot_closure", pathlib.Path(__file__).parents[1] / "tools/boot_closure.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class AliasContract(unittest.TestCase):
    def test_member_after_closed_sdk_namespace_is_not_aliased(self):
        source = '''namespace Sound { void helper() {} }
// @symbol _ZN6Player7IsInAirEv
extern "C" {
int Player::IsInAir() { return mIsAirborne; }
}
'''
        result, count = module.add_free_function_aliases(source)
        self.assertEqual(count, 0)
        self.assertNotIn("/alternatename", result)

    def test_sdk_free_function_still_has_alias(self):
        source = '''namespace Sound {
// @symbol _ZN5Sound4StopEv
void Stop() { }
}
'''
        result, count = module.add_free_function_aliases(source)
        self.assertEqual(count, 1)
        self.assertIn("__ZN5Sound4StopEv", result)

    def test_member_followed_by_call_does_not_alias_caller(self):
        source = '''// @symbol _ZN6Player19func_ov002_020c0434Ev
int Player::func_ov002_020c0434() { return 0; }
// @symbol func_ov002_020c1ad8
int func_ov002_020c1ad8(Player *p, int angle) {
    if (p->func_ov002_020c0434() != 0) { return 0; }
    return angle;
}
'''
        result, count = module.add_free_function_aliases(source)
        self.assertEqual(count, 1)
        self.assertNotIn("__ZN6Player19func_ov002_020c0434Ev=", result)
        self.assertIn("_func_ov002_020c1ad8=", result)

    def test_call_condition_is_not_a_definition(self):
        source = '''// @symbol func_02000000
int unrelated() { if (func_02000000() != 0) { return 1; } return 0; }
'''
        self.assertEqual(module.add_free_function_aliases(source), (source, 0))

    def test_aggregate_player_source_does_not_alias_movement_member(self):
        player = pathlib.Path(__file__).parents[2] / "src/actors/Player.cpp"
        result, _ = module.add_free_function_aliases(player.read_text())
        self.assertNotIn("__ZN6Player19func_ov002_020c0434Ev=", result)
        self.assertIn("_func_ov002_020c1ad8=", result)


if __name__ == "__main__":
    unittest.main()
