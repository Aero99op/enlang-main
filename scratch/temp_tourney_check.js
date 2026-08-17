
let currentView = "view-landing";

let currentWsTab = "panel-ws-overview";

let activeTourneyId = 1;

let activeMatchIdx = 0;

let editingTeamName = "";

let currentUser = { name : "Kiryu_FF" , uid : "77489210" , role : "Organizer" , loggedIn : true };

const tournamentsDb = [ { id : 1 , title : "VORTEX GRANDMASTERS CHAMPIONSHIP" , game : "Free Fire MAX" , format : "SQUAD (BR)" , maps : "Bermuda, Purgatory, Kalahari, Alpine" , slots : 12 , prize : "₹25,000" , status : "LIVE" , statusClass : "live" , killMultiplier : 1 , placementPoints : { "1" : 12 , "2" : 9 , "3" : 8 , "4" : 7 , "5" : 6 , "6" : 5 , "7" : 4 , "8" : 3 , "9" : 2 , "10" : 1 , "11" : 0 , "12" : 0 } , teams : [ { slot : 1 , name : "Shadow Ninjas" , tag : "SNE" , captain : "Kiryu_FF" , players : [ { name : "Kiryu_FF" , uid : "77489210" , role : "IGL (In-Game Leader)" } , { name : "Zen_99" , uid : "77489211" , role : "Entry Fragger / Rusher" } , { name : "Taro_X" , uid : "77489212" , role : "Support / Healer" } , { name : "Ken" , uid : "77489213" , role : "Sniper / Marksman" } ] } , { slot : 2 , name : "Aero Esports" , tag : "AERO" , captain : "Aero_Alpha" , players : [ { name : "Aero_Alpha" , uid : "66120101" , role : "IGL (In-Game Leader)" } , { name : "Aero_Sniper" , uid : "66120102" , role : "Sniper / Marksman" } , { name : "Aero_Ghost" , uid : "66120103" , role : "Entry Fragger / Rusher" } , { name : "Rex" , uid : "66120104" , role : "Support / Healer" } ] } , { slot : 3 , name : "Titan Squad" , tag : "TITAN" , captain : "Titan_Max" , players : [ { name : "Titan_Max" , uid : "5510101" , role : "IGL (In-Game Leader)" } , { name : "Titan_Bolt" , uid : "5510102" , role : "Entry Fragger / Rusher" } , { name : "Titan_Frost" , uid : "5510103" , role : "Support / Healer" } , { name : "Spike" , uid : "5510104" , role : "Sniper / Marksman" } ] } , { slot : 4 , name : "Nova Gaming" , tag : "NOVA" , captain : "Nova_Flash" , players : [ { name : "Nova_Flash" , uid : "4419010" , role : "IGL (In-Game Leader)" } , { name : "Nova_Strike" , uid : "4419011" , role : "Entry Fragger / Rusher" } , { name : "Nova_Viper" , uid : "4419012" , role : "Support / Healer" } ] } , { slot : 5 , name : "Phoenix Esports" , tag : "PHX" , captain : "Phx_Flame" , players : [ { name : "Phx_Flame" , uid : "3310001" , role : "IGL (In-Game Leader)" } , { name : "Phx_Blaze" , uid : "3310002" , role : "Entry Fragger / Rusher" } , { name : "Spark" , uid : "3310003" , role : "Support / Healer" } ] } , { slot : 6 , name : "GodLike Elite" , tag : "GDL" , captain : "God_Zeus" , players : [ { name : "God_Zeus" , uid : "2218001" , role : "IGL (In-Game Leader)" } , { name : "God_Thor" , uid : "2218002" , role : "Entry Fragger / Rusher" } , { name : "Ares" , uid : "2218003" , role : "Support / Healer" } ] } ] , matches : [ { id : 1 , title : "Match 1 - Bermuda Battle" , map : "Bermuda" , time : "8:00 PM IST" , roomId : "8849201" , roomPass : "VORTEX77" , status : "COMPLETED" , scores : [ { team : "Shadow Ninjas" , place : 1 , kills : 9 , bonus : 0 , penalty : 0 } , { team : "Aero Esports" , place : 2 , kills : 8 , bonus : 0 , penalty : 0 } , { team : "Titan Squad" , place : 3 , kills : 6 , bonus : 0 , penalty : 0 } , { team : "Nova Gaming" , place : 4 , kills : 5 , bonus : 0 , penalty : 0 } , { team : "Phoenix Esports" , place : 5 , kills : 4 , bonus : 0 , penalty : 0 } , { team : "GodLike Elite" , place : 6 , kills : 3 , bonus : 0 , penalty : 0 } ] } , { id : 2 , title : "Match 2 - Purgatory Clash" , map : "Purgatory" , time : "8:40 PM IST" , roomId : "8849202" , roomPass : "VORTEX88" , status : "LIVE" , scores : [ { team : "Aero Esports" , place : 1 , kills : 11 , bonus : 0 , penalty : 0 } , { team : "Shadow Ninjas" , place : 2 , kills : 7 , bonus : 0 , penalty : 0 } , { team : "Nova Gaming" , place : 3 , kills : 6 , bonus : 0 , penalty : 0 } , { team : "Titan Squad" , place : 4 , kills : 4 , bonus : 0 , penalty : 0 } , { team : "GodLike Elite" , place : 5 , kills : 3 , bonus : 0 , penalty : 0 } , { team : "Phoenix Esports" , place : 6 , kills : 2 , bonus : 0 , penalty : 0 } ] } , { id : 3 , title : "Match 3 - Kalahari Desert" , map : "Kalahari" , time : "9:20 PM IST" , roomId : "8849203" , roomPass : "VORTEX99" , status : "SCHEDULED" , scores : [ ] } ] , checkpoints : [ { title : "Initial Baseline (Before Match 1)" , timestamp : "8:00 PM IST" , standings : [ { team : "Shadow Ninjas" , played : 0 , wwcd : 0 , kills : 0 , killPts : 0 , placePts : 0 , totalPts : 0 } , { team : "Aero Esports" , played : 0 , wwcd : 0 , kills : 0 , killPts : 0 , placePts : 0 , totalPts : 0 } ] } , { title : "Post Match 1 Standings" , timestamp : "8:35 PM IST" , standings : [ { team : "Shadow Ninjas" , played : 1 , wwcd : 1 , kills : 9 , killPts : 9 , placePts : 12 , totalPts : 21 } , { team : "Aero Esports" , played : 1 , wwcd : 0 , kills : 8 , killPts : 8 , placePts : 9 , totalPts : 17 } , { team : "Titan Squad" , played : 1 , wwcd : 0 , kills : 6 , killPts : 6 , placePts : 8 , totalPts : 14 } ] } ] } , { id : 2 , title : "AERO PRO LEAGUE SEASON 4" , game : "Free Fire MAX" , format : "SQUAD (BR)" , maps : "Purgatory, Alpine, NexTerra" , slots : 12 , prize : "₹10,000" , status : "LIVE" , statusClass : "live" , killMultiplier : 1 , placementPoints : { "1" : 12 , "2" : 9 , "3" : 8 , "4" : 7 , "5" : 6 , "6" : 5 , "7" : 4 , "8" : 3 , "9" : 2 , "10" : 1 , "11" : 0 , "12" : 0 } , teams : [ { slot : 1 , name : "Aero Esports" , tag : "AERO" , captain : "Aero_Alpha" , players : [ { name : "Aero_Alpha" , uid : "66120101" , role : "IGL" } , { name : "Aero_Sniper" , uid : "66120102" , role : "Sniper" } ] } , { slot : 2 , name : "Dark Hunters" , tag : "DHK" , captain : "Hunter_07" , players : [ { name : "Hunter_07" , uid : "119001" , role : "IGL" } , { name : "Hunter_Wolf" , uid : "119002" , role : "Rusher" } ] } ] , matches : [ { id : 1 , title : "Match 1 - Purgatory" , map : "Purgatory" , time : "7:00 PM IST" , roomId : "9910441" , roomPass : "AERO99" , status : "COMPLETED" , scores : [ ] } ] , checkpoints : [ ] } , { id : 3 , title : "MIDNIGHT CLASH SCRIMS" , game : "Free Fire MAX" , format : "SQUAD (BR)" , maps : "Kalahari, Alpine" , slots : 12 , prize : "₹5,000" , status : "UPCOMING" , statusClass : "open" , killMultiplier : 1 , placementPoints : { "1" : 12 , "2" : 9 , "3" : 8 , "4" : 7 , "5" : 6 , "6" : 5 , "7" : 4 , "8" : 3 , "9" : 2 , "10" : 1 , "11" : 0 , "12" : 0 } , teams : [ ] , matches : [ ] , checkpoints : [ ] } ];

function showToast(message) {
  let container = document.getElementById ( "toast-container" );
  if (container != null) {
    let toast = document.createElement ( "div" );
    toast.className = "toast-item";
    toast.innerText = message;
    container.appendChild ( toast );
    setTimeout(function() {
      if (toast.parentNode != null) {
        toast.parentNode.removeChild ( toast );
      }
    }, 3000);
  }
}

function getActiveTourney() {
  for (const t of tournamentsDb) {
    if (t.id == activeTourneyId) {
      return t;
    }
  }
  return tournamentsDb [ 0 ];
}

function switchView(targetId) {
  currentView = targetId;
  let allSections = document.querySelectorAll ( ".view-section" );
  for (const sec of allSections) {
    sec.classList.remove ( "active" );
    sec.style.display = "none";
  }
  let targetSec = document.getElementById ( targetId );
  if (targetSec != null) {
    targetSec.classList.add ( "active" );
    targetSec.style.display = "block";
  }
  let navLinks = document.querySelectorAll ( ".nav-link" );
  for (const lnk of navLinks) {
    lnk.classList.remove ( "active" );
  }
  if (targetId == "view-landing") {
    let l1 = document.getElementById ( "nav-landing" );
    if (l1 != null) {
      l1.classList.add ( "active" );
    }
  }
  if (targetId == "view-create") {
    let l2 = document.getElementById ( "nav-create" );
    if (l2 != null) {
      l2.classList.add ( "active" );
    }
  }
  if (targetId == "view-manage") {
    let l3 = document.getElementById ( "nav-manage" );
    if (l3 != null) {
      l3.classList.add ( "active" );
    }
  }
  window.scrollTo ( 0 , 0 );
}

function switchWsTab(panelId) {
  currentWsTab = panelId;
  let allPanels = document.querySelectorAll ( ".ws-panel" );
  for (const p of allPanels) {
    p.classList.remove ( "active" );
    p.style.display = "none";
  }
  let targetPanel = document.getElementById ( panelId );
  if (targetPanel != null) {
    targetPanel.classList.add ( "active" );
    targetPanel.style.display = "block";
  }
  let allTabs = document.querySelectorAll ( ".ws-tab" );
  for (const tb of allTabs) {
    tb.classList.remove ( "active" );
  }
  if (panelId == "panel-ws-overview") {
    let t1 = document.getElementById ( "ws-tab-overview" );
    if (t1 != null) {
      t1.classList.add ( "active" );
    }
  }
  if (panelId == "panel-ws-teams") {
    let t2 = document.getElementById ( "ws-tab-teams" );
    if (t2 != null) {
      t2.classList.add ( "active" );
    }
  }
  if (panelId == "panel-ws-matches") {
    let t3 = document.getElementById ( "ws-tab-matches" );
    if (t3 != null) {
      t3.classList.add ( "active" );
    }
  }
  if (panelId == "panel-ws-match-standings") {
    let t4 = document.getElementById ( "ws-tab-match-standings" );
    if (t4 != null) {
      t4.classList.add ( "active" );
    }
  }
  if (panelId == "panel-ws-overall-standings") {
    let t5 = document.getElementById ( "ws-tab-overall-standings" );
    if (t5 != null) {
      t5.classList.add ( "active" );
    }
  }
  if (panelId == "panel-ws-points-rules") {
    let t6 = document.getElementById ( "ws-tab-points-rules" );
    if (t6 != null) {
      t6.classList.add ( "active" );
    }
  }
  if (panelId == "panel-ws-exports") {
    let t7 = document.getElementById ( "ws-tab-exports" );
    if (t7 != null) {
      t7.classList.add ( "active" );
    }
  }
}

function renderLandingFeatured() {
  let grid = document.getElementById ( "landing-tourney-grid" );
  if (grid != null) {
    let htmlBuffer = "";
    for (const tourney of tournamentsDb) {
      htmlBuffer = htmlBuffer + "<div class='tourney-card-item' onclick='window.vortexOpenWorkspace(" + tourney.id + ")'>";
      htmlBuffer = htmlBuffer + "<div class='card-top-row'>";
      htmlBuffer = htmlBuffer + "<span class='badge-tag " + tourney.statusClass + "'>" + tourney.status + "</span>";
      htmlBuffer = htmlBuffer + "<span class='badge-tag open'>" + tourney.format + "</span>";
      htmlBuffer = htmlBuffer + "</div>";
      htmlBuffer = htmlBuffer + "<div class='t-card-title'>" + tourney.title + "</div>";
      htmlBuffer = htmlBuffer + "<div class='t-card-meta'>Game: " + tourney.game + " • Maps: " + tourney.maps + "</div>";
      htmlBuffer = htmlBuffer + "<div class='t-card-metrics'>";
      htmlBuffer = htmlBuffer + "<div class='t-metric'><span class='tm-label'>PRIZE POOL</span><span class='tm-val highlight'>" + tourney.prize + "</span></div>";
      htmlBuffer = htmlBuffer + "<div class='t-metric'><span class='tm-label'>SQUADS</span><span class='tm-val'>" + tourney.teams.length + " / " + tourney.slots + "</span></div>";
      htmlBuffer = htmlBuffer + "</div>";
      htmlBuffer = htmlBuffer + "<button class='btn-action-primary' style='width:100%;'>OPEN WORKSPACE ➔</button>";
      htmlBuffer = htmlBuffer + "</div>";
    }
    grid.innerHTML = htmlBuffer;
  }
}

function renderManageList() {
  let grid = document.getElementById ( "manage-tournaments-grid" );
  if (grid != null) {
    let htmlBuffer = "";
    for (const tourney of tournamentsDb) {
      htmlBuffer = htmlBuffer + "<div class='tourney-card-item' onclick='window.vortexOpenWorkspace(" + tourney.id + ")'>";
      htmlBuffer = htmlBuffer + "<div class='card-top-row'>";
      htmlBuffer = htmlBuffer + "<span class='badge-tag " + tourney.statusClass + "'>" + tourney.status + "</span>";
      htmlBuffer = htmlBuffer + "<span class='badge-tag open'>" + tourney.game + "</span>";
      htmlBuffer = htmlBuffer + "</div>";
      htmlBuffer = htmlBuffer + "<div class='t-card-title'>" + tourney.title + "</div>";
      htmlBuffer = htmlBuffer + "<div class='t-card-meta'>Format: " + tourney.format + " • Maps: " + tourney.maps + "</div>";
      htmlBuffer = htmlBuffer + "<div class='t-card-metrics'>";
      htmlBuffer = htmlBuffer + "<div class='t-metric'><span class='tm-label'>PRIZE POOL</span><span class='tm-val highlight'>" + tourney.prize + "</span></div>";
      htmlBuffer = htmlBuffer + "<div class='t-metric'><span class='tm-label'>SQUADS REGISTERED</span><span class='tm-val'>" + tourney.teams.length + " / " + tourney.slots + "</span></div>";
      htmlBuffer = htmlBuffer + "<div class='t-metric'><span class='tm-label'>MATCHES</span><span class='tm-val'>" + tourney.matches.length + " Scheduled</span></div>";
      htmlBuffer = htmlBuffer + "<div class='t-metric'><span class='tm-label'>CHECKPOINTS</span><span class='tm-val'>" + tourney.checkpoints.length + " Saved</span></div>";
      htmlBuffer = htmlBuffer + "</div>";
      htmlBuffer = htmlBuffer + "<button class='btn-action-primary' style='width:100%;'>ENTER ORGANIZER WORKSPACE ➔</button>";
      htmlBuffer = htmlBuffer + "</div>";
    }
    grid.innerHTML = htmlBuffer;
  }
}

function openWorkspaceWithId(tourneyId) {
  activeTourneyId = tourneyId;
  let activeT = null;
  for (const t of tournamentsDb) {
    if (t.id == tourneyId) {
      activeT = t;
    }
  }
  if (activeT != null) {
    let titleEl = document.getElementById ( "ws-tourney-title" );
    if (titleEl != null) {
      titleEl.innerText = activeT.title;
    }
    let metaEl = document.getElementById ( "ws-game-meta" );
    if (metaEl != null) {
      metaEl.innerText = activeT.game + " • " + activeT.format + " • Prize: " + activeT.prize + " • Maps: " + activeT.maps;
    }
    let statusBadge = document.getElementById ( "ws-status-badge" );
    if (statusBadge != null) {
      statusBadge.innerText = activeT.status;
    }
    renderWorkspaceOverview();
    renderWorkspaceTeams();
    renderWorkspaceMatches();
    renderWorkspaceMatchStandings();
    renderWorkspaceOverallStandings();
    renderWorkspacePointRules();
    switchWsTab("panel-ws-overview");
    switchView("view-workspace");
  }
}

function renderWorkspaceOverview() {
  let activeT = getActiveTourney ( );
  if (activeT != null) {
    document.getElementById ( "stat-total-teams" ) .innerText = activeT.teams.length + " / " + activeT.slots;
    let completedMatches = 0;
    for (const m of activeT.matches) {
      if (m.status == "COMPLETED") {
        completedMatches = completedMatches + 1;
      }
    }
    document.getElementById ( "stat-matches-played" ) .innerText = completedMatches + " / " + activeT.matches.length;
    document.getElementById ( "stat-prize-pool" ) .innerText = activeT.prize;
    let overviewBody = document.getElementById ( "ws-overview-table-body" );
    if (overviewBody != null) {
      let overallList = computeOverallStandings ( activeT );
      if (overallList.length > 0) {
        document.getElementById ( "stat-table-leader" ) .innerText = overallList [ 0 ] .team + " (" + overallList [ 0 ] .totalPts + " PTS)";
      }
      let htmlBuffer = "";
      let rank = 1;
      for (const row of overallList) {
        let rankClass = "rank-badge";
        if (rank == 1) {
          rankClass = "rank-badge rank-1";
        }
        if (rank == 2) {
          rankClass = "rank-badge rank-2";
        }
        if (rank == 3) {
          rankClass = "rank-badge rank-3";
        }
        htmlBuffer = htmlBuffer + "<tr>";
        htmlBuffer = htmlBuffer + "<td><span class='" + rankClass + "'>#" + rank + "</span></td>";
        htmlBuffer = htmlBuffer + "<td><strong>" + row.team + "</strong></td>";
        htmlBuffer = htmlBuffer + "<td>" + row.played + "</td>";
        htmlBuffer = htmlBuffer + "<td>" + row.wwcd + "</td>";
        htmlBuffer = htmlBuffer + "<td>" + row.killPts + "</td>";
        htmlBuffer = htmlBuffer + "<td>" + row.placePts + "</td>";
        htmlBuffer = htmlBuffer + "<td><span class='total-pts-pill'>" + row.totalPts + " PTS</span></td>";
        htmlBuffer = htmlBuffer + "<td style='text-align:right;'><button class='btn-secondary-sm' style='padding:3px 8px; font-size:11px;' onclick='window.vortexOpenTeamMatchesModal(\"" + row.team + "\")'>✏️ EDIT MATCHES</button></td>";
        htmlBuffer = htmlBuffer + "</tr>";
        rank = rank + 1;
      }
      overviewBody.innerHTML = htmlBuffer;
    }
  }
}

function renderWorkspaceTeams() {
  let activeT = getActiveTourney ( );
  let container = document.getElementById ( "ws-teams-container" );
  if (activeT != null && container != null) {
    if (activeT.teams.length == 0) {
      container.innerHTML = "<div style='padding:32px; text-align:center; color:#64748b;'>No squads registered yet. Click '+ ADD NEW TEAM' to register squad slots.</div>";
      return 0;
    }
    let htmlBuffer = "";
    let tIdx = 0;
    for (const team of activeT.teams) {
      htmlBuffer = htmlBuffer + "<div class='team-roster-card'>";
      htmlBuffer = htmlBuffer + "<div class='team-roster-header'>";
      htmlBuffer = htmlBuffer + "<div class='team-title-group'>";
      htmlBuffer = htmlBuffer + "<span class='team-slot-badge'>SLOT " + team.slot + "</span>";
      htmlBuffer = htmlBuffer + "<span class='team-name-text'>" + team.name + "</span>";
      htmlBuffer = htmlBuffer + "<span class='team-tag-pill'>" + team.tag + "</span>";
      htmlBuffer = htmlBuffer + "<span style='font-size:12px; color:#94a3b8;'>Captain: " + team.captain + "</span>";
      htmlBuffer = htmlBuffer + "</div>";
      htmlBuffer = htmlBuffer + "<div class='team-actions-group'>";
      htmlBuffer = htmlBuffer + "<button class='btn-secondary-sm' onclick='window.vortexOpenTeamMatchesModal(\"" + team.name + "\")'>EDIT ALL MATCHES</button>";
      htmlBuffer = htmlBuffer + "<button class='btn-secondary-sm' onclick='window.vortexOpenAddPlayerModal(" + tIdx + ")'>+ ADD PLAYER</button>";
      htmlBuffer = htmlBuffer + "<button class='btn-secondary-sm' onclick='window.vortexEditTeamModal(" + tIdx + ")'>EDIT SQUAD</button>";
      htmlBuffer = htmlBuffer + "<button class='btn-row-del' onclick='window.vortexDeleteTeam(" + tIdx + ")'>REMOVE SQUAD</button>";
      htmlBuffer = htmlBuffer + "</div>";
      htmlBuffer = htmlBuffer + "</div>";
      htmlBuffer = htmlBuffer + "<div class='players-table-wrapper'>";
      htmlBuffer = htmlBuffer + "<table class='anime-table'>";
      htmlBuffer = htmlBuffer + "<thead><tr><th>PLAYER IGN</th><th>FREE FIRE UID</th><th>SQUAD ROLE</th><th style='text-align:right;'>PLAYER ACTIONS</th></tr></thead>";
      htmlBuffer = htmlBuffer + "<tbody>";
      if (team.players == undefined || team.players.length == 0) {
        htmlBuffer = htmlBuffer + "<tr><td colspan='4' style='color:#64748b; text-align:center;'>No players added to this squad roster yet.</td></tr>";
      }
      else {
        let pIdx = 0;
        for (const player of team.players) {
          htmlBuffer = htmlBuffer + "<tr>";
          htmlBuffer = htmlBuffer + "<td><strong>" + player.name + "</strong></td>";
          htmlBuffer = htmlBuffer + "<td style='font-family:monospace; color:#00f0ff;'>" + player.uid + "</td>";
          htmlBuffer = htmlBuffer + "<td><span class='player-role-badge'>" + player.role + "</span></td>";
          htmlBuffer = htmlBuffer + "<td style='text-align:right;'>";
          htmlBuffer = htmlBuffer + "<button class='btn-secondary-sm' style='padding:2px 8px; margin-right:4px;' onclick='window.vortexEditPlayerModal(" + tIdx + ", " + pIdx + ")'>EDIT</button>";
          htmlBuffer = htmlBuffer + "<button class='btn-row-del' style='padding:2px 8px;' onclick='window.vortexDeletePlayer(" + tIdx + ", " + pIdx + ")'>REMOVE</button>";
          htmlBuffer = htmlBuffer + "</td>";
          htmlBuffer = htmlBuffer + "</tr>";
          pIdx = pIdx + 1;
        }
      }
      htmlBuffer = htmlBuffer + "</tbody></table></div></div>";
      tIdx = tIdx + 1;
    }
    container.innerHTML = htmlBuffer;
  }
}

function renderWorkspaceMatches() {
  let activeT = getActiveTourney ( );
  let grid = document.getElementById ( "ws-matches-grid" );
  if (activeT != null && grid != null) {
    if (activeT.matches.length == 0) {
      grid.innerHTML = "<div style='padding:32px; text-align:center; color:#64748b;'>No matches scheduled yet. Click '+ SCHEDULE NEW MATCH' to create brackets.</div>";
      return 0;
    }
    let htmlBuffer = "";
    for (const matchItem of activeT.matches) {
      let statusBadge = "open";
      if (matchItem.status == "LIVE") {
        statusBadge = "live";
      }
      if (matchItem.status == "COMPLETED") {
        statusBadge = "completed";
      }
      htmlBuffer = htmlBuffer + "<div class='tourney-card-item' style='cursor:default;'>";
      htmlBuffer = htmlBuffer + "<div class='card-top-row'>";
      htmlBuffer = htmlBuffer + "<span class='badge-tag " + statusBadge + "'>" + matchItem.status + "</span>";
      htmlBuffer = htmlBuffer + "<span class='badge-tag open'>" + matchItem.map + "</span>";
      htmlBuffer = htmlBuffer + "</div>";
      htmlBuffer = htmlBuffer + "<div class='t-card-title'>" + matchItem.title + "</div>";
      htmlBuffer = htmlBuffer + "<div class='t-card-meta'>Scheduled Time: " + matchItem.time + "</div>";
      htmlBuffer = htmlBuffer + "<div class='t-card-metrics'>";
      htmlBuffer = htmlBuffer + "<div class='t-metric'><span class='tm-label'>CUSTOM ROOM ID</span><span class='tm-val highlight' style='letter-spacing:1px;'>" + matchItem.roomId + "</span></div>";
      htmlBuffer = htmlBuffer + "<div class='t-metric'><span class='tm-label'>ROOM PASSWORD</span><span class='tm-val' style='letter-spacing:1px;'>" + matchItem.roomPass + "</span></div>";
      htmlBuffer = htmlBuffer + "</div>";
      htmlBuffer = htmlBuffer + "<button class='btn-secondary-sm' style='width:100%;' onclick='window.vortexToggleMatchStatus(" + matchItem.id + ")'>TOGGLE STATUS (SCHEDULED / LIVE / DONE)</button>";
      htmlBuffer = htmlBuffer + "</div>";
    }
    grid.innerHTML = htmlBuffer;
  }
}

function renderWorkspaceMatchStandings() {
  let activeT = getActiveTourney ( );
  if (activeT != null) {
    let selectEl = document.getElementById ( "ws-match-select-dropdown" );
    if (selectEl != null) {
      let optBuffer = "";
      let mIdx = 0;
      for (const m of activeT.matches) {
        let selectedAttr = "";
        if (mIdx == activeMatchIdx) {
          selectedAttr = " selected";
        }
        optBuffer = optBuffer + "<option value='" + mIdx + "'" + selectedAttr + ">" + m.title + " (" + m.status + ")</option>";
        mIdx = mIdx + 1;
      }
      selectEl.innerHTML = optBuffer;
    }
    let activeMatch = activeT.matches [ activeMatchIdx ];
    if (activeMatch != undefined) {
      document.getElementById ( "ws-active-match-title" ) .innerText = activeMatch.title;
      document.getElementById ( "ws-active-match-status" ) .innerText = activeMatch.status;
      if (activeMatch.scores.length == 0) {
        let initRank = 1;
        for (const teamItem of activeT.teams) {
          activeMatch.scores.push ( { team : teamItem.name , place : initRank , kills : 0 , bonus : 0 , penalty : 0 } );
          initRank = initRank + 1;
        }
      }
      let tbody = document.getElementById ( "ws-match-standings-tbody" );
      if (tbody != null) {
        let htmlBuffer = "";
        let sIdx = 0;
        for (const scoreRow of activeMatch.scores) {
          let pKey = String ( scoreRow.place );
          let placePts = 0;
          if (activeT.placementPoints [ pKey ] != undefined) {
            placePts = activeT.placementPoints [ pKey ];
          }
          let killPts = Number ( scoreRow.kills ) * Number ( activeT.killMultiplier );
          let totalPts = placePts + killPts + Number ( scoreRow.bonus ) - Number ( scoreRow.penalty );
          htmlBuffer = htmlBuffer + "<tr>";
          htmlBuffer = htmlBuffer + "<td><strong class='rank-badge'>#" + ( sIdx + 1 ) + "</strong></td>";
          htmlBuffer = htmlBuffer + "<td><strong>" + scoreRow.team + "</strong></td>";
          htmlBuffer = htmlBuffer + "<td><input class='table-edit-input' type='number' min='1' max='12' value='" + scoreRow.place + "' onchange='window.vortexUpdateMatchScore(" + sIdx + ", \"place\", this.value)'></td>";
          htmlBuffer = htmlBuffer + "<td><input class='table-edit-input' type='number' min='0' max='50' value='" + scoreRow.kills + "' onchange='window.vortexUpdateMatchScore(" + sIdx + ", \"kills\", this.value)'></td>";
          htmlBuffer = htmlBuffer + "<td>" + killPts + "</td>";
          htmlBuffer = htmlBuffer + "<td>" + placePts + "</td>";
          htmlBuffer = htmlBuffer + "<td><input class='table-edit-input' type='number' min='0' max='20' value='" + scoreRow.bonus + "' onchange='window.vortexUpdateMatchScore(" + sIdx + ", \"bonus\", this.value)'></td>";
          htmlBuffer = htmlBuffer + "<td><input class='table-edit-input' type='number' min='0' max='20' value='" + scoreRow.penalty + "' onchange='window.vortexUpdateMatchScore(" + sIdx + ", \"penalty\", this.value)'></td>";
          htmlBuffer = htmlBuffer + "<td><span class='total-pts-pill'>" + totalPts + " PTS</span></td>";
          htmlBuffer = htmlBuffer + "<td>";
          htmlBuffer = htmlBuffer + "<button class='btn-secondary-sm' style='padding:2px 6px; margin-right:4px; font-size:10px;' onclick='window.vortexOpenTeamMatchesModal(\"" + scoreRow.team + "\")'>ALL MATCHES</button>";
          htmlBuffer = htmlBuffer + "<button class='btn-row-del' onclick='window.vortexDeleteMatchRow(" + sIdx + ")'>DEL</button>";
          htmlBuffer = htmlBuffer + "</td>";
          htmlBuffer = htmlBuffer + "</tr>";
          sIdx = sIdx + 1;
        }
        tbody.innerHTML = htmlBuffer;
      }
    }
  }
}

function computeOverallStandings(activeT) {
  let teamMap = { };
  for (const teamItem of activeT.teams) {
    teamMap[teamItem.name] = { team: teamItem.name, played: 0, wwcd: 0, kills: 0, killPts: 0, placePts: 0, totalPts: 0 }
  }
  for (const m of activeT.matches) {
    if (m.status == "COMPLETED" || m.status == "LIVE") {
      for (const sc of m.scores) {
        if (teamMap [ sc.team ] == undefined) {
          teamMap[sc.team] = { team: sc.team, played: 0, wwcd: 0, kills: 0, killPts: 0, placePts: 0, totalPts: 0 }
        }
        let record = teamMap [ sc.team ];
        record.played = record.played + 1;
        if (Number ( sc.place ) == 1) {
          record.wwcd = record.wwcd + 1;
        }
        let pKey = String ( sc.place );
        let pPts = 0;
        if (activeT.placementPoints [ pKey ] != undefined) {
          pPts = activeT.placementPoints [ pKey ];
        }
        let kPts = Number ( sc.kills ) * Number ( activeT.killMultiplier );
        record.kills = record.kills + Number ( sc.kills );
        record.killPts = record.killPts + kPts;
        record.placePts = record.placePts + pPts;
        record.totalPts = record.totalPts + pPts + kPts + Number ( sc.bonus ) - Number ( sc.penalty );
      }
    }
  }
  let resultList = [ ];
  for (const k of Object.keys ( teamMap )) {
    resultList.push ( teamMap [ k ] );
  }
  resultList.sort ( function ( itemA , itemB ) { return itemB.totalPts - itemA.totalPts } );
  return resultList;
}

function renderWorkspaceOverallStandings() {
  let activeT = getActiveTourney ( );
  let tbody = document.getElementById ( "ws-overall-standings-tbody" );
  if (activeT != null && tbody != null) {
    let overallList = computeOverallStandings ( activeT );
    let htmlBuffer = "";
    let rank = 1;
    for (const row of overallList) {
      let rankClass = "rank-badge";
      if (rank == 1) {
        rankClass = "rank-badge rank-1";
      }
      if (rank == 2) {
        rankClass = "rank-badge rank-2";
      }
      if (rank == 3) {
        rankClass = "rank-badge rank-3";
      }
      htmlBuffer = htmlBuffer + "<tr>";
      htmlBuffer = htmlBuffer + "<td><span class='" + rankClass + "'>#" + rank + "</span></td>";
      htmlBuffer = htmlBuffer + "<td><strong>" + row.team + "</strong></td>";
      htmlBuffer = htmlBuffer + "<td>" + row.played + "</td>";
      htmlBuffer = htmlBuffer + "<td>" + row.wwcd + "</td>";
      htmlBuffer = htmlBuffer + "<td>" + row.kills + "</td>";
      htmlBuffer = htmlBuffer + "<td>" + row.killPts + "</td>";
      htmlBuffer = htmlBuffer + "<td>" + row.placePts + "</td>";
      htmlBuffer = htmlBuffer + "<td><span class='total-pts-pill'>" + row.totalPts + " PTS</span></td>";
      htmlBuffer = htmlBuffer + "<td style='text-align:right;'><button class='btn-action-primary-sm' style='padding:4px 10px; font-size:11px;' onclick='window.vortexOpenTeamMatchesModal(\"" + row.team + "\")'>✏️ EDIT ALL MATCHES</button></td>";
      htmlBuffer = htmlBuffer + "</tr>";
      rank = rank + 1;
    }
    tbody.innerHTML = htmlBuffer;
  }
}

function openTeamMatchesModal(targetTeam) {
  editingTeamName = targetTeam;
  let activeT = getActiveTourney ( );
  let tbody = document.getElementById ( "modal-team-matches-tbody" );
  if (activeT != null && tbody != null) {
    document.getElementById ( "modal-team-matches-title" ) .innerText = "EDIT ALL MATCH SCORES — " + targetTeam;
    let htmlBuffer = "";
    let mIdx = 0;
    for (const m of activeT.matches) {
      let placeVal = 12;
      let killsVal = 0;
      let bonusVal = 0;
      let penaltyVal = 0;
      for (const sc of m.scores) {
        if (sc.team == targetTeam) {
          placeVal = Number ( sc.place );
          killsVal = Number ( sc.kills );
          bonusVal = Number ( sc.bonus );
          penaltyVal = Number ( sc.penalty );
        }
      }
      let pKey = String ( placeVal );
      let placePts = 0;
      if (activeT.placementPoints [ pKey ] != undefined) {
        placePts = activeT.placementPoints [ pKey ];
      }
      let killPts = killsVal * Number ( activeT.killMultiplier );
      let totalMatchPts = placePts + killPts + bonusVal - penaltyVal;
      let statusBadge = "open";
      if (m.status == "LIVE") {
        statusBadge = "live";
      }
      if (m.status == "COMPLETED") {
        statusBadge = "completed";
      }
      htmlBuffer = htmlBuffer + "<tr>";
      htmlBuffer = htmlBuffer + "<td><strong>" + m.title + "</strong><br><span style='font-size:11px; color:#64748b;'>Map: " + m.map + "</span></td>";
      htmlBuffer = htmlBuffer + "<td><span class='badge-tag " + statusBadge + "'>" + m.status + "</span></td>";
      htmlBuffer = htmlBuffer + "<td><input type='number' min='1' max='12' class='table-edit-input' id='modal-m-place-" + mIdx + "' value='" + placeVal + "' oninput='window.vortexCalcTeamModalLive()'></td>";
      htmlBuffer = htmlBuffer + "<td><input type='number' min='0' max='50' class='table-edit-input' id='modal-m-kills-" + mIdx + "' value='" + killsVal + "' oninput='window.vortexCalcTeamModalLive()'></td>";
      htmlBuffer = htmlBuffer + "<td><input type='number' min='0' max='20' class='table-edit-input' id='modal-m-bonus-" + mIdx + "' value='" + bonusVal + "' oninput='window.vortexCalcTeamModalLive()'></td>";
      htmlBuffer = htmlBuffer + "<td><input type='number' min='0' max='20' class='table-edit-input' id='modal-m-penalty-" + mIdx + "' value='" + penaltyVal + "' oninput='window.vortexCalcTeamModalLive()'></td>";
      htmlBuffer = htmlBuffer + "<td><span class='total-pts-pill' id='modal-m-pts-" + mIdx + "'>" + totalMatchPts + " PTS</span></td>";
      htmlBuffer = htmlBuffer + "</tr>";
      mIdx = mIdx + 1;
    }
    tbody.innerHTML = htmlBuffer;
    calcTeamModalLive();
    document.getElementById ( "modal-team-matches-edit" ) .classList.add ( "show" );
  }
}

function calcTeamModalLive() {
  let activeT = getActiveTourney ( );
  if (activeT != null) {
    let cumPlayed = 0;
    let cumWwcd = 0;
    let cumKills = 0;
    let cumKillPts = 0;
    let cumPlacePts = 0;
    let cumTotalPts = 0;
    let mIdx = 0;
    for (const m of activeT.matches) {
      let pEl = document.getElementById ( "modal-m-place-" + mIdx );
      let kEl = document.getElementById ( "modal-m-kills-" + mIdx );
      let bEl = document.getElementById ( "modal-m-bonus-" + mIdx );
      let penEl = document.getElementById ( "modal-m-penalty-" + mIdx );
      let ptsEl = document.getElementById ( "modal-m-pts-" + mIdx );
      if (pEl != null && kEl != null) {
        let pVal = Number ( pEl.value );
        let kVal = Number ( kEl.value );
        let bVal = Number ( bEl.value );
        let penVal = Number ( penEl.value );
        let pKey = String ( pVal );
        let placePts = 0;
        if (activeT.placementPoints [ pKey ] != undefined) {
          placePts = activeT.placementPoints [ pKey ];
        }
        let killPts = kVal * Number ( activeT.killMultiplier );
        let rowTotal = placePts + killPts + bVal - penVal;
        if (ptsEl != null) {
          ptsEl.innerText = rowTotal + " PTS";
        }
        cumPlayed = cumPlayed + 1;
        if (pVal == 1) {
          cumWwcd = cumWwcd + 1;
        }
        cumKills = cumKills + kVal;
        cumKillPts = cumKillPts + killPts;
        cumPlacePts = cumPlacePts + placePts;
        cumTotalPts = cumTotalPts + rowTotal;
      }
      mIdx = mIdx + 1;
    }
    let summaryEl = document.getElementById ( "team-modal-stats-summary" );
    if (summaryEl != null) {
      let sumHtml = "";
      sumHtml = sumHtml + "<div class='tm-stat-box'><span class='tm-stat-lbl'>MATCHES</span><span class='tm-stat-val'>" + cumPlayed + " / " + activeT.matches.length + "</span></div>";
      sumHtml = sumHtml + "<div class='tm-stat-box'><span class='tm-stat-lbl'>BOOYAH (WWCD)</span><span class='tm-stat-val highlight'>" + cumWwcd + "</span></div>";
      sumHtml = sumHtml + "<div class='tm-stat-box'><span class='tm-stat-lbl'>TOTAL KILLS</span><span class='tm-stat-val'>" + cumKills + " (" + cumKillPts + " PTS)</span></div>";
      sumHtml = sumHtml + "<div class='tm-stat-box'><span class='tm-stat-lbl'>PLACEMENT PTS</span><span class='tm-stat-val'>" + cumPlacePts + " PTS</span></div>";
      sumHtml = sumHtml + "<div class='tm-stat-box'><span class='tm-stat-lbl'>NEW OVERALL TOTAL</span><span class='tm-stat-val super'>" + cumTotalPts + " PTS</span></div>";
      summaryEl.innerHTML = sumHtml;
    }
  }
}

function saveTeamAllMatches() {
  let activeT = getActiveTourney ( );
  if (activeT != null && editingTeamName != "") {
    let mIdx = 0;
    for (const m of activeT.matches) {
      let pEl = document.getElementById ( "modal-m-place-" + mIdx );
      let kEl = document.getElementById ( "modal-m-kills-" + mIdx );
      let bEl = document.getElementById ( "modal-m-bonus-" + mIdx );
      let penEl = document.getElementById ( "modal-m-penalty-" + mIdx );
      if (pEl != null && kEl != null) {
        let pVal = Number ( pEl.value );
        let kVal = Number ( kEl.value );
        let bVal = Number ( bEl.value );
        let penVal = Number ( penEl.value );
        let found = false;
        for (const sc of m.scores) {
          if (sc.team == editingTeamName) {
            sc.place = pVal;
            sc.kills = kVal;
            sc.bonus = bVal;
            sc.penalty = penVal;
            found = true;
          }
        }
        if (found == false) {
          m.scores.push ( { team : editingTeamName , place : pVal , kills : kVal , bonus : bVal , penalty : penVal } );
        }
        m.scores.sort ( function ( itemA , itemB ) { return itemA.place - itemB.place } );
      }
      mIdx = mIdx + 1;
    }
    document.getElementById ( "modal-team-matches-edit" ) .classList.remove ( "show" );
    renderWorkspaceOverview();
    renderWorkspaceMatches();
    renderWorkspaceMatchStandings();
    renderWorkspaceOverallStandings();
    showToast("✓ Saved & auto-calculated all matches for " + editingTeamName + "! Standings updated live.");
  }
}

function renderWorkspacePointRules() {
  let activeT = getActiveTourney ( );
  let grid = document.getElementById ( "ws-rules-pts-grid" );
  if (activeT != null && grid != null) {
    document.getElementById ( "ws-rules-kill-pts" ) .value = activeT.killMultiplier;
    let htmlBuffer = "";
    for (const r of [ 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 , 10 , 11 , 12 ]) {
      let val = 0;
      if (activeT.placementPoints [ String ( r ) ] != undefined) {
        val = activeT.placementPoints [ String ( r ) ];
      }
      htmlBuffer = htmlBuffer + "<div class='pt-box'>";
      htmlBuffer = htmlBuffer + "<span class='pt-lbl'>#" + r + " Rank</span>";
      htmlBuffer = htmlBuffer + "<input type='number' class='pt-input' id='ws-pt-rank-" + r + "' value='" + val + "'>";
      htmlBuffer = htmlBuffer + "</div>";
    }
    grid.innerHTML = htmlBuffer;
  }
}

function createStandingsCheckpoint(customTitle) {
  let activeT = getActiveTourney ( );
  if (activeT != null) {
    let overallSnapshot = computeOverallStandings ( activeT );
    let titleText = customTitle;
    if (titleText == undefined || titleText == "") {
      titleText = "Manual Checkpoint #" + ( activeT.checkpoints.length + 1 );
    }
    let timeStr = new Date ( ) .toLocaleTimeString ( );
    activeT.checkpoints.push ( { title : titleText , timestamp : timeStr , standings : JSON.parse ( JSON.stringify ( overallSnapshot ) ) } );
    showToast("🔖 Checkpoint saved: " + titleText);
  }
}

function renderRevertModalList() {
  let activeT = getActiveTourney ( );
  let list = document.getElementById ( "revert-checkpoints-list" );
  if (activeT != null && list != null) {
    if (activeT.checkpoints.length == 0) {
      list.innerHTML = "<div style='color:#64748b; text-align:center; padding:16px;'>No checkpoints recorded yet for this tournament.</div>";
      return 0;
    }
    let htmlBuffer = "";
    let cIdx = 0;
    for (const cp of activeT.checkpoints) {
      htmlBuffer = htmlBuffer + "<div class='tourney-card-item' style='margin-bottom:10px; padding:12px; cursor:default;'>";
      htmlBuffer = htmlBuffer + "<div style='display:flex; justify-content:space-between; align-items:center;'>";
      htmlBuffer = htmlBuffer + "<div><strong>" + cp.title + "</strong><br><span style='font-size:11px; color:#64748b;'>Saved At: " + cp.timestamp + " (" + cp.standings.length + " Squads)</span></div>";
      htmlBuffer = htmlBuffer + "<button class='btn-revert-sm' onclick='window.vortexApplyRevert(" + cIdx + ")'>RESTORE THIS STATE</button>";
      htmlBuffer = htmlBuffer + "</div>";
      htmlBuffer = htmlBuffer + "</div>";
      cIdx = cIdx + 1;
    }
    list.innerHTML = htmlBuffer;
  }
}

function downloadTournamentCSV() {
  let activeT = getActiveTourney ( );
  if (activeT != null) {
    let overallList = computeOverallStandings ( activeT );
    let csvContent = "Overall_Rank,Squad,Matches_Played,Booyah_WWCD,Total_Kills,Kill_Points,Placement_Points,Total_Pointsn";
    let rank = 1;
    for (const row of overallList) {
      csvContent = csvContent + rank + "," + row.team + "," + row.played + "," + row.wwcd + "," + row.kills + "," + row.killPts + "," + row.placePts + "," + row.totalPts + "n";
      rank = rank + 1;
    }
    let blob = new Blob ( [ csvContent ] , { type : "text/csv;charset=utf-8;" } );
    let link = document.createElement ( "a" );
    let url = URL.createObjectURL ( blob );
    link.setAttribute ( "href" , url );
    link.setAttribute ( "download" , activeT.title.replaceAll ( " " , "_" ) + "_Standings.csv" );
    document.body.appendChild ( link );
    link.click ( );
    document.body.removeChild ( link );
    showToast("📥 Full Tournament CSV Downloaded!");
  }
}

function copyTextLeaderboardReport() {
  let activeT = getActiveTourney ( );
  if (activeT != null) {
    let overallList = computeOverallStandings ( activeT );
    let report = "🏆 " + activeT.title + " 🏆n";
    report = report + "🎮 " + activeT.game + " • Format: " + activeT.format + " • Prize: " + activeT.prize + "n";
    report = report + "═══════════════════════════════════════nn";
    let rank = 1;
    for (const row of overallList) {
      report = report + "#" + rank + " " + row.team + " | WWCD: " + row.wwcd + " | Kills: " + row.kills + " | Total: " + row.totalPts + " PTSn";
      rank = rank + 1;
    }
    report = report + "n═══════════════════════════════════════nGenerated via Vortex Esports OS";
    navigator.clipboard.writeText ( report );
    showToast("📋 Formatted Text Report copied to clipboard!");
  }
}

function editTeamModal(teamIdx) {
  let activeT = getActiveTourney ( );
  if (activeT != null && activeT.teams [ teamIdx ] != undefined) {
    let sq = activeT.teams [ teamIdx ];
    document.getElementById ( "edit-team-idx" ) .value = teamIdx;
    document.getElementById ( "team-input-slot" ) .value = sq.slot;
    document.getElementById ( "team-input-tag" ) .value = sq.tag;
    document.getElementById ( "team-input-name" ) .value = sq.name;
    document.getElementById ( "team-input-captain" ) .value = sq.captain;
    document.getElementById ( "modal-team-title" ) .innerText = "EDIT SQUAD DETAILS";
    document.getElementById ( "modal-team-edit" ) .classList.add ( "show" );
  }
}

function deleteTeam(teamIdx) {
  let activeT = getActiveTourney ( );
  if (activeT != null && activeT.teams [ teamIdx ] != undefined) {
    let name = activeT.teams [ teamIdx ] .name;
    activeT.teams.splice ( teamIdx , 1 );
    renderWorkspaceOverview();
    renderWorkspaceTeams();
    renderWorkspaceMatchStandings();
    renderWorkspaceOverallStandings();
    showToast("Removed squad '" + name + "' from tournament.");
  }
}

function openAddPlayerModal(teamIdx) {
  document.getElementById ( "edit-player-team-idx" ) .value = teamIdx;
  document.getElementById ( "edit-player-idx" ) .value = "-1";
  document.getElementById ( "player-input-name" ) .value = "";
  document.getElementById ( "player-input-uid" ) .value = "";
  document.getElementById ( "modal-player-title" ) .innerText = "ADD PLAYER TO SQUAD";
  document.getElementById ( "modal-player-edit" ) .classList.add ( "show" );
}

function editPlayerModal(teamIdx, playerIdx) {
  let activeT = getActiveTourney ( );
  if (activeT != null && activeT.teams [ teamIdx ] != undefined) {
    let p = activeT.teams [ teamIdx ] .players [ playerIdx ];
    if (p != undefined) {
      document.getElementById ( "edit-player-team-idx" ) .value = teamIdx;
      document.getElementById ( "edit-player-idx" ) .value = playerIdx;
      document.getElementById ( "player-input-name" ) .value = p.name;
      document.getElementById ( "player-input-uid" ) .value = p.uid;
      document.getElementById ( "player-input-role" ) .value = p.role;
      document.getElementById ( "modal-player-title" ) .innerText = "EDIT PLAYER ROSTER";
      document.getElementById ( "modal-player-edit" ) .classList.add ( "show" );
    }
  }
}

function deletePlayer(teamIdx, playerIdx) {
  let activeT = getActiveTourney ( );
  if (activeT != null && activeT.teams [ teamIdx ] != undefined) {
    let pName = activeT.teams [ teamIdx ] .players [ playerIdx ] .name;
    activeT.teams[teamIdx].players.splice(playerIdx, 1)
    renderWorkspaceTeams();
    showToast("Removed player '" + pName + "' from squad.");
  }
}

function toggleMatchStatus(matchId) {
  let activeT = getActiveTourney ( );
  if (activeT != null) {
    for (const m of activeT.matches) {
      if (m.id == matchId) {
        if (m.status == "SCHEDULED") {
          m.status = "LIVE";
        }
        else if (m.status == "LIVE") {
          m.status = "COMPLETED";
        }
        else {
          m.status = "SCHEDULED";
        }
        renderWorkspaceOverview();
        renderWorkspaceMatches();
        renderWorkspaceMatchStandings();
        renderWorkspaceOverallStandings();
        showToast(m.title + " status changed to: " + m.status);
      }
    }
  }
}

function updateMatchScore(scoreIdx, field, val) {
  let activeT = getActiveTourney ( );
  if (activeT != null && activeT.matches [ activeMatchIdx ] != undefined) {
    let row = activeT.matches [ activeMatchIdx ] .scores [ scoreIdx ];
    if (row != undefined) {
      row[field] = Number(val)
      activeT.matches[activeMatchIdx].scores.sort(function(itemA, itemB) { return itemA.place - itemB.place; })
      renderWorkspaceOverview();
      renderWorkspaceMatchStandings();
      renderWorkspaceOverallStandings();
      showToast("✓ Updated " + row.team + " " + field + " to " + val);
    }
  }
}

function deleteMatchRow(scoreIdx) {
  let activeT = getActiveTourney ( );
  if (activeT != null && activeT.matches [ activeMatchIdx ] != undefined) {
    activeT.matches[activeMatchIdx].scores.splice(scoreIdx, 1)
    renderWorkspaceMatchStandings();
    renderWorkspaceOverallStandings();
    showToast("Removed squad score row.");
  }
}

function applyRevert(checkpointIdx) {
  let activeT = getActiveTourney ( );
  if (activeT != null && activeT.checkpoints [ checkpointIdx ] != undefined) {
    let cp = activeT.checkpoints [ checkpointIdx ];
    let tbody = document.getElementById ( "ws-overall-standings-tbody" );
    if (tbody != null) {
      let htmlBuffer = "";
      let rank = 1;
      for (const row of cp.standings) {
        htmlBuffer = htmlBuffer + "<tr>";
        htmlBuffer = htmlBuffer + "<td><strong class='rank-badge'>#" + rank + "</strong></td>";
        htmlBuffer = htmlBuffer + "<td><strong>" + row.team + "</strong></td>";
        htmlBuffer = htmlBuffer + "<td>" + row.played + "</td>";
        htmlBuffer = htmlBuffer + "<td>" + row.wwcd + "</td>";
        htmlBuffer = htmlBuffer + "<td>" + row.kills + "</td>";
        htmlBuffer = htmlBuffer + "<td>" + row.killPts + "</td>";
        htmlBuffer = htmlBuffer + "<td>" + row.placePts + "</td>";
        htmlBuffer = htmlBuffer + "<td><span class='total-pts-pill'>" + row.totalPts + " PTS</span></td>";
        htmlBuffer = htmlBuffer + "<td style='text-align:right;'><button class='btn-action-primary-sm' style='padding:4px 10px; font-size:11px;' onclick='window.vortexOpenTeamMatchesModal(\"" + row.team + "\")'>✏️ EDIT ALL MATCHES</button></td>";
        htmlBuffer = htmlBuffer + "</tr>";
        rank = rank + 1;
      }
      tbody.innerHTML = htmlBuffer;
    }
    document.getElementById ( "modal-revert-standings" ) .classList.remove ( "show" );
    showToast("⏪ Successfully reverted standings to: " + cp.title);
  }
}

window.vortexOpenWorkspace = function ( id ) { openWorkspaceWithId ( id ) };

window.vortexEditTeamModal = function ( idx ) { editTeamModal ( idx ) };

window.vortexDeleteTeam = function ( idx ) { deleteTeam ( idx ) };

window.vortexOpenAddPlayerModal = function ( idx ) { openAddPlayerModal ( idx ) };

window.vortexEditPlayerModal = function ( tIdx , pIdx ) { editPlayerModal ( tIdx , pIdx ) };

window.vortexDeletePlayer = function ( tIdx , pIdx ) { deletePlayer ( tIdx , pIdx ) };

window.vortexToggleMatchStatus = function ( id ) { toggleMatchStatus ( id ) };

window.vortexUpdateMatchScore = function ( sIdx , fld , val ) { updateMatchScore ( sIdx , fld , val ) };

window.vortexDeleteMatchRow = function ( sIdx ) { deleteMatchRow ( sIdx ) };

window.vortexApplyRevert = function ( idx ) { applyRevert ( idx ) };

window.vortexOpenTeamMatchesModal = function ( team ) { openTeamMatchesModal ( team ) };

window.vortexCalcTeamModalLive = function ( ) { calcTeamModalLive ( ) };

(function() {
  const targetEl = (document.getElementById("btn-nav-brand") || document.querySelector("btn-nav-brand"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      switchView("view-landing");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("nav-landing") || document.querySelector("nav-landing"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      switchView("view-landing");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("nav-create") || document.querySelector("nav-create"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      switchView("view-create");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("nav-manage") || document.querySelector("nav-manage"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      switchView("view-manage");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("card-act-create") || document.querySelector("card-act-create"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      switchView("view-create");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("card-act-manage") || document.querySelector("card-act-manage"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      switchView("view-manage");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-landing-view-all") || document.querySelector("btn-landing-view-all"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      switchView("view-manage");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-manage-to-create") || document.querySelector("btn-manage-to-create"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      switchView("view-create");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-back-create-landing") || document.querySelector("btn-back-create-landing"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      switchView("view-landing");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-ws-back-to-manage") || document.querySelector("btn-ws-back-to-manage"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      switchView("view-manage");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("card-act-auth") || document.querySelector("card-act-auth"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      document.getElementById ( "modal-auth" ) .classList.add ( "show" );
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-open-auth") || document.querySelector("btn-open-auth"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      document.getElementById ( "modal-auth" ) .classList.add ( "show" );
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-close-auth-modal") || document.querySelector("btn-close-auth-modal"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      document.getElementById ( "modal-auth" ) .classList.remove ( "show" );
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-cancel-auth") || document.querySelector("btn-cancel-auth"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      document.getElementById ( "modal-auth" ) .classList.remove ( "show" );
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-confirm-auth") || document.querySelector("btn-confirm-auth"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let uname = document.getElementById ( "auth-username" ) .value;
      let uidVal = document.getElementById ( "auth-uid" ) .value;
      let roleVal = document.getElementById ( "auth-role" ) .value;
      currentUser.name = uname;
      currentUser.uid = uidVal;
      currentUser.role = roleVal;
      currentUser.loggedIn = true;
      document.getElementById ( "display-user-name" ) .innerText = uname + " (" + roleVal + ")";
      document.getElementById ( "user-profile-badge" ) .style.display = "flex";
      document.getElementById ( "btn-open-auth" ) .style.display = "none";
      document.getElementById ( "modal-auth" ) .classList.remove ( "show" );
      showToast("🛡️ Authenticated as " + uname + " [" + roleVal + "]!");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-logout-act") || document.querySelector("btn-logout-act"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      currentUser.loggedIn = false;
      document.getElementById ( "user-profile-badge" ) .style.display = "none";
      document.getElementById ( "btn-open-auth" ) .style.display = "inline-block";
      showToast("Logged out successfully.");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("ws-tab-overview") || document.querySelector("ws-tab-overview"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      switchWsTab("panel-ws-overview");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("ws-tab-teams") || document.querySelector("ws-tab-teams"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      switchWsTab("panel-ws-teams");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("ws-tab-matches") || document.querySelector("ws-tab-matches"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      switchWsTab("panel-ws-matches");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("ws-tab-match-standings") || document.querySelector("ws-tab-match-standings"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      switchWsTab("panel-ws-match-standings");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("ws-tab-overall-standings") || document.querySelector("ws-tab-overall-standings"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      switchWsTab("panel-ws-overall-standings");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("ws-tab-points-rules") || document.querySelector("ws-tab-points-rules"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      switchWsTab("panel-ws-points-rules");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("ws-tab-exports") || document.querySelector("ws-tab-exports"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      switchWsTab("panel-ws-exports");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("link-view-full-overall") || document.querySelector("link-view-full-overall"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      switchWsTab("panel-ws-overall-standings");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("quick-act-add-team") || document.querySelector("quick-act-add-team"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      switchWsTab("panel-ws-teams");
      document.getElementById ( "edit-team-idx" ) .value = "-1";
      document.getElementById ( "team-input-slot" ) .value = "7";
      document.getElementById ( "team-input-tag" ) .value = "";
      document.getElementById ( "team-input-name" ) .value = "";
      document.getElementById ( "team-input-captain" ) .value = "";
      document.getElementById ( "modal-team-edit" ) .classList.add ( "show" );
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("quick-act-new-match") || document.querySelector("quick-act-new-match"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      switchWsTab("panel-ws-matches");
      document.getElementById ( "modal-match-edit" ) .classList.add ( "show" );
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("quick-act-edit-points") || document.querySelector("quick-act-edit-points"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      switchWsTab("panel-ws-match-standings");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("quick-act-download-csv") || document.querySelector("quick-act-download-csv"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      downloadTournamentCSV();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-export-full-csv") || document.querySelector("btn-export-full-csv"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      downloadTournamentCSV();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-export-text-report") || document.querySelector("btn-export-text-report"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      copyTextLeaderboardReport();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-open-add-team-modal") || document.querySelector("btn-open-add-team-modal"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      document.getElementById ( "edit-team-idx" ) .value = "-1";
      document.getElementById ( "team-input-slot" ) .value = String ( tournamentsDb [ 0 ] .teams.length + 1 );
      document.getElementById ( "team-input-tag" ) .value = "";
      document.getElementById ( "team-input-name" ) .value = "";
      document.getElementById ( "team-input-captain" ) .value = "";
      document.getElementById ( "modal-team-title" ) .innerText = "ADD NEW SQUAD";
      document.getElementById ( "modal-team-edit" ) .classList.add ( "show" );
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-close-team-modal") || document.querySelector("btn-close-team-modal"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      document.getElementById ( "modal-team-edit" ) .classList.remove ( "show" );
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-cancel-team") || document.querySelector("btn-cancel-team"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      document.getElementById ( "modal-team-edit" ) .classList.remove ( "show" );
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-save-team") || document.querySelector("btn-save-team"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let activeT = getActiveTourney ( );
      let editIdx = Number ( document.getElementById ( "edit-team-idx" ) .value );
      let slotVal = Number ( document.getElementById ( "team-input-slot" ) .value );
      let tagVal = document.getElementById ( "team-input-tag" ) .value;
      let nameVal = document.getElementById ( "team-input-name" ) .value;
      let capVal = document.getElementById ( "team-input-captain" ) .value;
      if (nameVal == "") {
        nameVal = "Alpha Wolves";
      }
      if (tagVal == "") {
        tagVal = "AW";
      }
      if (capVal == "") {
        capVal = "Wolf_Alpha (UID: 8810291)";
      }
      if (activeT != null) {
        if (editIdx >= 0) {
          activeT.teams[editIdx].slot = slotVal
          activeT.teams[editIdx].tag = tagVal
          activeT.teams[editIdx].name = nameVal
          activeT.teams[editIdx].captain = capVal
          showToast("✓ Squad " + nameVal + " updated!");
        }
        else {
          activeT.teams.push ( { slot : slotVal , tag : tagVal , name : nameVal , captain : capVal , players : [ { name : capVal.split ( " " ) [ 0 ] , uid : "8810291" , role : "IGL" } ] } );
          showToast("✓ Squad " + nameVal + " added to Slot " + slotVal + "!");
        }
        renderWorkspaceOverview();
        renderWorkspaceTeams();
        renderWorkspaceMatchStandings();
        renderWorkspaceOverallStandings();
        document.getElementById ( "modal-team-edit" ) .classList.remove ( "show" );
      }
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-close-player-modal") || document.querySelector("btn-close-player-modal"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      document.getElementById ( "modal-player-edit" ) .classList.remove ( "show" );
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-cancel-player") || document.querySelector("btn-cancel-player"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      document.getElementById ( "modal-player-edit" ) .classList.remove ( "show" );
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-save-player") || document.querySelector("btn-save-player"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let activeT = getActiveTourney ( );
      let tIdx = Number ( document.getElementById ( "edit-player-team-idx" ) .value );
      let pIdx = Number ( document.getElementById ( "edit-player-idx" ) .value );
      let pName = document.getElementById ( "player-input-name" ) .value;
      let pUid = document.getElementById ( "player-input-uid" ) .value;
      let pRole = document.getElementById ( "player-input-role" ) .value;
      if (pName == "") {
        pName = "Striker_99";
      }
      if (pUid == "") {
        pUid = String ( Math.floor ( 10000000 + Math.random ( ) * 90000000 ) );
      }
      if (activeT != null && activeT.teams [ tIdx ] != undefined) {
        if (pIdx >= 0) {
          activeT.teams[tIdx].players[pIdx].name = pName
          activeT.teams[tIdx].players[pIdx].uid = pUid
          activeT.teams[tIdx].players[pIdx].role = pRole
          showToast("✓ Player " + pName + " updated!");
        }
        else {
          activeT.teams[tIdx].players.push({ name: pName, uid: pUid, role: pRole })
          showToast("✓ Added " + pName + " to " + activeT.teams [ tIdx ] .name + " roster!");
        }
        renderWorkspaceTeams();
        document.getElementById ( "modal-player-edit" ) .classList.remove ( "show" );
      }
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-open-add-match-modal") || document.querySelector("btn-open-add-match-modal"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      document.getElementById ( "modal-match-edit" ) .classList.add ( "show" );
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-close-match-modal") || document.querySelector("btn-close-match-modal"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      document.getElementById ( "modal-match-edit" ) .classList.remove ( "show" );
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-cancel-match") || document.querySelector("btn-cancel-match"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      document.getElementById ( "modal-match-edit" ) .classList.remove ( "show" );
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-save-match") || document.querySelector("btn-save-match"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let activeT = getActiveTourney ( );
      let mTitle = document.getElementById ( "match-input-title" ) .value;
      let mMap = document.getElementById ( "match-input-map" ) .value;
      let mTime = document.getElementById ( "match-input-time" ) .value;
      let mRoomId = document.getElementById ( "match-input-room-id" ) .value;
      let mPass = document.getElementById ( "match-input-room-pass" ) .value;
      if (mTitle == "") {
        mTitle = "Match " + ( activeT.matches.length + 1 ) + " - " + mMap;
      }
      if (mRoomId == "") {
        mRoomId = String ( Math.floor ( 1000000 + Math.random ( ) * 9000000 ) );
      }
      if (mPass == "") {
        mPass = "VORTEX2026";
      }
      if (activeT != null) {
        activeT.matches.push ( { id : activeT.matches.length + 1 , title : mTitle , map : mMap , time : mTime , roomId : mRoomId , roomPass : mPass , status : "SCHEDULED" , scores : [ ] } );
        renderWorkspaceOverview();
        renderWorkspaceMatches();
        renderWorkspaceMatchStandings();
        document.getElementById ( "modal-match-edit" ) .classList.remove ( "show" );
        showToast("🎮 Custom Match scheduled & Room ID " + mRoomId + " generated!");
      }
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-close-team-matches-modal") || document.querySelector("btn-close-team-matches-modal"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      document.getElementById ( "modal-team-matches-edit" ) .classList.remove ( "show" );
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-cancel-team-matches") || document.querySelector("btn-cancel-team-matches"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      document.getElementById ( "modal-team-matches-edit" ) .classList.remove ( "show" );
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-save-team-matches") || document.querySelector("btn-save-team-matches"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      saveTeamAllMatches();
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-ws-save-match-results") || document.querySelector("btn-ws-save-match-results"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      renderWorkspaceOverallStandings();
      renderWorkspaceOverview();
      showToast("💾 Match scores saved and overall leaderboard recalculated!");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-ws-publish-match") || document.querySelector("btn-ws-publish-match"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let activeT = getActiveTourney ( );
      if (activeT != null && activeT.matches [ activeMatchIdx ] != undefined) {
        activeT.matches[activeMatchIdx].status = "COMPLETED"
        createStandingsCheckpoint(( "Snapshot After " + activeT.matches [ activeMatchIdx ] .title ));
        renderWorkspaceOverview();
        renderWorkspaceMatches();
        renderWorkspaceMatchStandings();
        renderWorkspaceOverallStandings();
        showToast("📢 Match published and official standings checkpoint created!");
      }
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-ws-create-checkpoint") || document.querySelector("btn-ws-create-checkpoint"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      createStandingsCheckpoint("Manual Standings Checkpoint");
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-ws-open-revert-modal") || document.querySelector("btn-ws-open-revert-modal"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      renderRevertModalList();
      document.getElementById ( "modal-revert-standings" ) .classList.add ( "show" );
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-close-revert-modal") || document.querySelector("btn-close-revert-modal"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      document.getElementById ( "modal-revert-standings" ) .classList.remove ( "show" );
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-cancel-revert") || document.querySelector("btn-cancel-revert"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      document.getElementById ( "modal-revert-standings" ) .classList.remove ( "show" );
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-ws-save-point-rules") || document.querySelector("btn-ws-save-point-rules"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let activeT = getActiveTourney ( );
      if (activeT != null) {
        activeT.killMultiplier = Number ( document.getElementById ( "ws-rules-kill-pts" ) .value );
        for (const r of [ 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 , 10 , 11 , 12 ]) {
          let el = document.getElementById ( "ws-pt-rank-" + r );
          if (el != null) {
            activeT.placementPoints[String(r)] = Number(el.value)
          }
        }
        renderWorkspaceOverview();
        renderWorkspaceMatchStandings();
        renderWorkspaceOverallStandings();
        showToast("✓ Point system rules updated and standings recalculated!");
      }
    });
  }
})();

(function() {
  const targetEl = (document.getElementById("btn-submit-create-tourney") || document.querySelector("btn-submit-create-tourney"));
  if (targetEl != null) {
    targetEl.addEventListener('click', function(event) {
      let tTitle = document.getElementById ( "new-tourney-title" ) .value;
      let tGame = document.getElementById ( "new-tourney-game" ) .value;
      let tFormat = document.getElementById ( "new-tourney-format" ) .value;
      let tSlots = Number ( document.getElementById ( "new-tourney-slots" ) .value );
      let tPrize = document.getElementById ( "new-tourney-prize" ) .value;
      let tMaps = document.getElementById ( "new-tourney-maps" ) .value;
      let tKillMultiplier = Number ( document.getElementById ( "new-pts-kill" ) .value );
      if (tTitle == "") {
        tTitle = "VORTEX CLASH TOURNAMENT S1";
      }
      if (tPrize == "") {
        tPrize = "₹15,000";
      }
      if (tMaps == "") {
        tMaps = "Bermuda, Purgatory, Kalahari";
      }
      let customPlacementMap = { };
      for (const r of [ 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 , 10 , 11 , 12 ]) {
        let ptEl = document.getElementById ( "pt-rank-" + r );
        if (ptEl != null) {
          customPlacementMap[String(r)] = Number(ptEl.value)
        }
        else {
          customPlacementMap[String(r)] = 0
        }
      }
      let newId = tournamentsDb.length + 1;
      let newTourney = { id : newId , title : tTitle , game : tGame , format : tFormat , maps : tMaps , slots : tSlots , prize : tPrize , status : "LIVE" , statusClass : "live" , killMultiplier : tKillMultiplier , placementPoints : customPlacementMap , teams : [ { slot : 1 , name : "Shadow Ninjas" , tag : "SNE" , captain : "Kiryu_FF" , players : [ { name : "Kiryu_FF" , uid : "77489210" , role : "IGL" } , { name : "Zen_99" , uid : "77489211" , role : "Rusher" } ] } , { slot : 2 , name : "Aero Esports" , tag : "AERO" , captain : "Aero_Alpha" , players : [ { name : "Aero_Alpha" , uid : "66120101" , role : "IGL" } , { name : "Aero_Sniper" , uid : "66120102" , role : "Sniper" } ] } ] , matches : [ { id : 1 , title : "Match 1 - " + tMaps.split ( "," ) [ 0 ] , map : tMaps.split ( "," ) [ 0 ] , time : "8:00 PM IST" , roomId : String ( Math.floor ( 1000000 + Math.random ( ) * 9000000 ) ) , roomPass : "VORTEX2026" , status : "SCHEDULED" , scores : [ ] } ] , checkpoints : [ ] };
      tournamentsDb.unshift ( newTourney );
      renderLandingFeatured();
      renderManageList();
      openWorkspaceWithId(newId);
      showToast("🚀 Tournament '" + tTitle + "' successfully created & launched!");
    });
  }
})();

renderLandingFeatured();

renderManageList();

openWorkspaceWithId(1);

switchView("view-landing");
    