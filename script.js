// Import Firebase SDK modules
import { initializeApp } from "https://www.gstatic.com/firebasejs/11.0.1/firebase-app.js";
import { getDatabase, ref, onValue } from "https://www.gstatic.com/firebasejs/11.0.1/firebase-database.js";

// Your Firebase config (from console)
const firebaseConfig = {
  apiKey: "AIzaSyB5GKSdrHjKjUwsTYt_ksmvrINWjS7xYHg",
  databaseURL: "https://compost-bin-alert-syste-default-rtdb.firebaseio.com/",
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);
const database = getDatabase(app);

// Reference to your data
const distanceRef = ref(database, "Sensor/data");

// Listen for changes in real time
onValue(distanceRef, (snapshot) => {
  const value = snapshot.val();
  const display = document.getElementById("distance");

  if (value === null) {
    display.textContent = "No data yet...";
  } else if (value < 0) {
    display.textContent = "Out of range";
  } else {
    display.textContent = `${value.toFixed(2)} cm`;
  }
});
