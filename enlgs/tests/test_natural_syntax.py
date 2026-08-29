"""Test Suite for Natural English Syntax in Enlang Script (.enlgs).

Validates 'put ... into', 'update ... to', 'add ... to list',
'remove item at ... from list', and 'insert ... at ... in list'.
"""

import unittest
from enlgs.compiler import compile_enlgs_source

class TestNaturalEnglishSyntax(unittest.TestCase):

    def test_put_into_dictionary_and_array(self):
        source = """
in script:
    create teamMap as {}
    put { kills: 16 } into teamMap["Shadow Ninjas"]
    put 50 into scores[0]
    put 16 into record.kills
"""
        js = compile_enlgs_source(source)
        self.assertIn('teamMap [ "Shadow Ninjas" ] = { kills : 16 };', js)
        self.assertIn('scores [ 0 ] = 50;', js)
        self.assertIn('record.kills = 16;', js)

    def test_put_at_index(self):
        source = """
in script:
    put 99 at 2 in matrixRow
    put "Finals" at index 3 in tournamentRounds
"""
        js = compile_enlgs_source(source)
        self.assertIn('matrixRow[2] = 99;', js)
        self.assertIn('tournamentRounds[3] = "Finals";', js)

    def test_update_to(self):
        source = """
in script:
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
    set kills of record to 16
    set prize of activeT to "50000"
"""
        js = compile_enlgs_source(source)
        self.assertIn('record.kills = 16;', js)
        self.assertIn('activeT.prize = "50000";', js)

    def test_list_add_and_push(self):
        source = """
in script:
    add teamItem to activeT.teams
    add "Shadow Ninjas" to squadList
    push 100 to pointsHistory
"""
        js = compile_enlgs_source(source)
        self.assertIn('activeT.teams.push(teamItem);', js)
        self.assertIn('squadList.push("Shadow Ninjas");', js)
        self.assertIn('pointsHistory.push(100);', js)

    def test_list_remove_at(self):
        source = """
in script:
    remove item at tIdx from activeT.teams
    remove at 2 from scoresList
    remove 0 from leaderboard
"""
        js = compile_enlgs_source(source)
        self.assertIn('activeT.teams.splice(tIdx, 1);', js)
        self.assertIn('scoresList.splice(2, 1);', js)
        self.assertIn('leaderboard.splice(0, 1);', js)

    def test_list_insert_at(self):
        source = """
in script:
    insert newMatch at 0 in activeT.matches
    insert "Alpha" at index 1 in namesList
    insert bonusCard into rewardsDeck at 2
"""
        js = compile_enlgs_source(source)
        self.assertIn('activeT.matches.splice(0, 0, newMatch);', js)
        self.assertIn('namesList.splice(1, 0, "Alpha");', js)
        self.assertIn('rewardsDeck.splice(2, 0, bonusCard);', js)

    def test_dom_operations_remain_unaffected(self):
        source = """
in script:
    add class "active" to "nav-item"
    remove class "open" from "sidebar"
    add element "div" with class "card"
"""
        js = compile_enlgs_source(source)
        self.assertIn(".classList.add('active');", js)
        self.assertIn(".classList.remove('open');", js)
        self.assertIn("document.createElement('div')", js)

if __name__ == '__main__':
    unittest.main()
