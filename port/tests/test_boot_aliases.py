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


if __name__ == "__main__":
    unittest.main()
