let counter = 0;

const APP_NAME = "Enlang Full-Stack App";

let isDark = false;

console.log("Initializing " + APP_NAME);

function greet(user) {
  console.log("Welcome, " + user);
  return "Hello " + user;
}

function incrementCounter() {
  counter += 1;
  (document.getElementById("counter-display") || document.querySelector("counter-display")).textContent = counter;
  if (counter > 5) {
    console.log("Counter reached milestone!");
    (document.getElementById("counter-display") || document.querySelector("counter-display")).style.color = "#38bdf8";
  }
  else {
    (document.getElementById("counter-display") || document.querySelector("counter-display")).style.color = "white";
  }
}

(document.getElementById("inc-btn") || document.querySelector("inc-btn")).addEventListener('click', function(event) {
  incrementCounter();
});

(document.getElementById("toggle-btn") || document.querySelector("toggle-btn")).addEventListener('click', function(event) {
  if (isDark == false) {
    document.body.classList.add('dark-theme');
    (document.getElementById("toggle-btn") || document.querySelector("toggle-btn")).textContent = "Light Mode";
    isDark = true;
  }
  else {
    document.body.classList.remove('dark-theme');
    (document.getElementById("toggle-btn") || document.querySelector("toggle-btn")).textContent = "Dark Mode";
    isDark = false;
  }
});

(document.getElementById("login-form") || document.querySelector("login-form")).addEventListener('submit', function(event) {
  if (typeof event !== 'undefined' && event.preventDefault) event.preventDefault();
  let username = (document.getElementById("username-input") || document.querySelector("username-input")).value;
  if (username == "") {
    alert("Please enter username");
    return;
  }
  (document.getElementById("status") || document.querySelector("status")).textContent = "Logging in...";
  (async function() {
    try {
      const response = await fetch('https://jsonplaceholder.typicode.com/todos/1');
      const data = await response.json();
      (document.getElementById("status") || document.querySelector("status")).textContent = "Logged in as " + username;
      localStorage.setItem('active_user', username);
    } catch (err) {
      console.error('Fetch error:', err);
    }
  })();
});

setTimeout(function() {
  (document.getElementById("initial-loader") || document.querySelector("initial-loader")).style.display = 'none';
  (document.getElementById("main-content") || document.querySelector("main-content")).style.display = '';
}, 2000);

for (let _i1 = 0; _i1 < 3; _i1++) {
  console.log("Enlang Script is Active!");
}

const customMap = new Map ( );

customMap.set("version", "1.0.0");

console.log("Raw JS Map entry:", customMap.get("version"));