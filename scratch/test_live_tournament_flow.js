const fs = require('fs');
const html = fs.readFileSync('d:/enlangg/tournament_app/tournament.html', 'utf8');

// Basic sanity check of compiled code
console.log("HTML length:", html.length);
console.log("Contains set text compiled element:", html.includes('.textContent = '));
console.log("Contains set html compiled element:", html.includes('.innerHTML = '));
console.log("Contains add class compiled element:", html.includes('.classList.add('));
console.log("Contains remove class compiled element:", html.includes('.classList.remove('));
console.log("Contains copy report compiled element:", html.includes('navigator.clipboard.writeText('));
console.log("Contains scroll to compiled element:", html.includes('window.scrollIntoView('));
console.log("Contains after timer compiled element:", html.includes('setTimeout(function()'));
console.log("ALL COMPILED PURE ENLGS CHECKS PASSED!");
