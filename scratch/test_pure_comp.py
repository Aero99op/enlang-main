import sys
import os
sys.path.insert(0, os.path.abspath("."))
from enlgs.lexer import ENLGSLexer
from enlgs.parser import ENLGSParser
from enlgs.emitter import ENLGSEmitter

sample = """
in script:
    to do computeOverallStandings with activeT:
        create teamMap as {}
        for each teamItem in activeT.teams:
            teamMap[teamItem.name] = { team: teamItem.name, played: 0, wwcd: 0, kills: 0, killPts: 0, placePts: 0, totalPts: 0 }

        for each m in activeT.matches:
            if m.status == "COMPLETED" || m.status == "LIVE":
                for each sc in m.scores:
                    if teamMap[sc.team] == undefined:
                        teamMap[sc.team] = { team: sc.team, played: 0, wwcd: 0, kills: 0, killPts: 0, placePts: 0, totalPts: 0 }

                    create record as teamMap[sc.team]
                    set record.played = record.played + 1
                    if Number(sc.place) == 1:
                        set record.wwcd = record.wwcd + 1

                    create pKey as String(sc.place)
                    create pPts as 0
                    if activeT.placementPoints[pKey] != undefined:
                        set pPts = activeT.placementPoints[pKey]

                    create kPts as Number(sc.kills) * Number(activeT.killMultiplier)
                    set record.kills = record.kills + Number(sc.kills)
                    set record.killPts = record.killPts + kPts
                    set record.placePts = record.placePts + pPts
                    set record.totalPts = record.totalPts + pPts + kPts + Number(sc.bonus) - Number(sc.penalty)

        create resultList as []
        for each k in Object.keys(teamMap):
            resultList.push(teamMap[k])

        resultList.sort(function(itemA, itemB) { return itemB.totalPts - itemA.totalPts; })
        return resultList

    to do switchView with targetId:
        set currentView = targetId
        hide element "view-landing"
        hide element "view-create"
        hide element "view-manage"
        hide element "view-workspace"

        remove class "active" from "nav-landing"
        remove class "active" from "nav-create"
        remove class "active" from "nav-manage"

        if targetId == "view-landing":
            show element "view-landing"
            add class "active" to "view-landing"
            add class "active" to "nav-landing"

        if targetId == "view-create":
            show element "view-create"
            add class "active" to "view-create"
            add class "active" to "nav-create"

        if targetId == "view-manage":
            show element "view-manage"
            add class "active" to "view-manage"
            add class "active" to "nav-manage"

        if targetId == "view-workspace":
            show element "view-workspace"
            add class "active" to "view-workspace"

        scroll to "main-navbar"
"""

lexer = ENLGSLexer(sample)
tokens = lexer.tokenize()
parser = ENLGSParser(tokens)
ast = parser.parse()
emitter = ENLGSEmitter(ast)
js_out = emitter.emit()

print("--- COMPILED JS ---")
print(js_out)
