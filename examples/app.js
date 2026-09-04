let count = 0;

let isDarkMode = true;

let totalTasks = 0;

console.log("⚡ Enlang Full-Stack Engine initialized successfully");

function updateCounterDisplay() {
  (document.getElementById("counter-val") || document.querySelector("counter-val")).textContent = count;
  if (count > 10) {
    (document.getElementById("counter-val") || document.querySelector("counter-val")).style.color = "#f43f5e";
    (document.getElementById("counter-badge") || document.querySelector("counter-badge")).textContent = "High Velocity 🔥";
  }
  else if (count < 0) {
    (document.getElementById("counter-val") || document.querySelector("counter-val")).style.color = "#fbbf24";
    (document.getElementById("counter-badge") || document.querySelector("counter-badge")).textContent = "Negative Range ❄️";
  }
  else {
    (document.getElementById("counter-val") || document.querySelector("counter-val")).style.color = "#38bdf8";
    (document.getElementById("counter-badge") || document.querySelector("counter-badge")).textContent = "Optimal Level ✨";
  }
}

function toggleTheme() {
  if (isDarkMode == true) {
    document.body.classList.remove('dark-theme');
    document.body.classList.add('light-theme');
    (document.getElementById("theme-btn") || document.querySelector("theme-btn")).textContent = "☀️ Light Mode";
    isDarkMode = false;
  }
  else {
    document.body.classList.remove('light-theme');
    document.body.classList.add('dark-theme');
    (document.getElementById("theme-btn") || document.querySelector("theme-btn")).textContent = "🌙 Dark Mode";
    isDarkMode = true;
  }
}

(document.getElementById("theme-btn") || document.querySelector("theme-btn")).addEventListener('click', function(event) {
  toggleTheme();
});

(document.getElementById("btn-inc") || document.querySelector("btn-inc")).addEventListener('click', function(event) {
  count += 1;
  updateCounterDisplay();
});

(document.getElementById("btn-dec") || document.querySelector("btn-dec")).addEventListener('click', function(event) {
  count -= 1;
  updateCounterDisplay();
});

(document.getElementById("btn-reset") || document.querySelector("btn-reset")).addEventListener('click', function(event) {
  count = 0;
  updateCounterDisplay();
});

(document.getElementById("task-form") || document.querySelector("task-form")).addEventListener('submit', function(event) {
  if (typeof event !== 'undefined' && event.preventDefault) event.preventDefault();
  let taskName = (document.getElementById("task-input") || document.querySelector("task-input")).value;
  if (taskName == "") {
    alert("Please enter a task name!");
    return;
  }
  totalTasks += 1;
  (document.getElementById("task-count") || document.querySelector("task-count")).textContent = totalTasks;
  (document.getElementById("task-status") || document.querySelector("task-status")).textContent = "Added: " + taskName;
  (document.getElementById("task-input") || document.querySelector("task-input")).value = "";
});

(document.getElementById("api-btn") || document.querySelector("api-btn")).addEventListener('click', function(event) {
  (document.getElementById("api-status") || document.querySelector("api-status")).textContent = "📡 Fetching live post from API...";
  (async function() {
    try {
      const response = await fetch('https://jsonplaceholder.typicode.com/posts/1');
      const data = await response.json();
      (document.getElementById("api-status") || document.querySelector("api-status")).textContent = "✅ Received: " + data.title;
      console.log("API Response Title: " + data.title);
    } catch (err) {
      console.error('Fetch error:', err);
    }
  })();
});

setInterval(function() {
  (document.getElementById("live-clock") || document.querySelector("live-clock")).textContent = "Engine Active • " + new Date ( ) .toLocaleTimeString ( );
}, 1000);

console.log("Natural English Scripting (.enlgs) is 100% active!");