"""Test Suite for Natural English Syntax in Enlang Script (.enlgs).

Validates `put ... into`, `update ... to`, `add ... to list`, `remove item at ... from list`,
`insert ... at ... in list`, and ensures existing DOM and variable assignment remain 100% backward compatible.
"""

import unittest
from enlgs.compiler import compile_enlgs_source

class TestNaturalEnglishSyntax(unittest.TestCase):

    def test_put_into_dictionary_map(self):
        source = """
in script:
    create teamMap as {}
    put { kills: 16 } into teamMap["Shadow Ninjas"]
"""
        js = compile_enlgs_source(source)
        self.assertIn('teamMap [ "Shadow Ninjas" ] = { kills : 16 };', js)

    def test_put_into_array_index(self):
        source = """
in script:
    create scores as [10, 20, 30]
    put 50 into scores[0]
"""
        js = compile_enlgs_source(source)
        self.assertIn('scores [ 0 ] = 50;', js)

    def test_put_at_index_in_list(self):
        source = """
in script:
    create scores as [10, 20, 30]
    put 99 at index 2 in scores
"""
        js = compile_enlgs_source(source)
        self.assertIn('scores[2] = 99;', js)

    def test_put_into_object_property(self):
        source = """
in script:
    create record as { kills: 0 }
    put 16 into record.kills
"""
        js = compile_enlgs_source(source)
        self.assertIn('record.kills = 16;', js)

    def test_update_to_target(self):
        source = """
in script:
    create teamMap as {}
    update teamMap["Aero"] to { kills: 8 }
    update scores[1] to 75
    update totalScore to 150
    update record.placePts to 12
"""
        js = compile_enlgs_source(source)
        self.assertIn('teamMap [ "Aero" ] = { kills : 8 };', js)
        self.assertIn('scores [ 1 ] = 75;', js)
        self.assertIn('totalScore = 150;', js)
        self.assertIn('record.placePts = 12;', js)

    def test_set_property_of_target(self):
        source = """
in script:
    create record as {}
    set kills of record to 16
    set prize of activeT to "₹50,000"
"""
        js = compile_enlgs_source(source)
        self.assertIn('record.kills = 16;', js)
        self.assertIn('activeT.prize = "₹50,000";', js)

    def test_add_and_push_to_list(self):
        source = """
in script:
    create teams as []
    add "Shadow Ninjas" to teams
    push 100 to pointsList
"""
        js = compile_enlgs_source(source)
        self.assertIn('teams.push("Shadow Ninjas");', js)
        self.assertIn('pointsList.push(100);', js)

    def test_remove_item_from_list(self):
        source = """
in script:
    create teams as ["A", "B", "C"]
    remove item at 1 from teams
    remove at 0 from scoresList
"""
        js = compile_enlgs_source(source)
        self.assertIn('teams.splice(1, 1);', js)
        self.assertIn('scoresList.splice(0, 1);', js)

    def test_insert_into_list(self):
        source = """
in script:
    create matches as []
    insert newMatch at 0 in matches
    insert "Alpha" at index 1 in namesList
"""
        js = compile_enlgs_source(source)
        self.assertIn('matches.splice(0, 0, newMatch);', js)
        self.assertIn('namesList.splice(1, 0, "Alpha");', js)

    def test_dom_class_no_collision(self):
        source = """
in script:
    add class "active" to "nav-item"
    remove class "open" from "sidebar"
    add element "div" with class "card"
"""
        js = compile_enlgs_source(source)
        self.assertIn(".classList.add('active')", js)
        self.assertIn(".classList.remove('open')", js)
        self.assertIn("document.createElement('div')", js)

if __name__ == "__main__":
    unittest.main()
