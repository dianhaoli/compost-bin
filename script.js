// Import Firebase SDK modules
import { initializeApp } from "https://www.gstatic.com/firebasejs/11.0.1/firebase-app.js";
import { getDatabase, ref, onValue } from "https://www.gstatic.com/firebasejs/11.0.1/firebase-database.js";

// --- Firebase Config ---
const firebaseConfig = {
  apiKey: "AIzaSyB5GKSdrHjKjUwsTYt_ksmvrINWjS7xYHg",
  databaseURL: "https://compost-bin-alert-syste-default-rtdb.firebaseio.com/",
};

// --- Initialize Firebase ---
const app = initializeApp(firebaseConfig);
const database = getDatabase(app);

// --- Bin name mapping ---
const binNames = {
  "bin_206D37004F8C": "Compost Bin 1 - Backyard",
  "binac67b2ff0a4c": "Compost Bin 2 - North Corner",
  "binbc5f9d3e14c2": "Compost Bin 3 - Greenhouse",
  "bin9f85d6a0e221": "Compost Bin 4 - Compost Zone"
};

// --- DOM Setup ---
const binsContainer = document.getElementById("bins");

// --- Safety check for container ---
if (!binsContainer) {
  console.error('❌ Error: No element found with id="bins" in HTML.');
} else {
  // Listen for all bins under Sensor/data
  const dataRef = ref(database, "Sensor/data");

  onValue(dataRef, (snapshot) => {
    const data = snapshot.val();
    binsContainer.innerHTML = ""; // Clear old data

    // Check if data is valid
    if (!data || typeof data !== "object") {
      binsContainer.innerHTML = "<p>No data available...</p>";
      return;
    }

    // Loop through all bins found in Firebase
    Object.keys(data).forEach((binID) => {
      const binData = data[binID] || {};
      const name = binNames[binID] || binID; // fallback to raw ID if no mapping
      const distanceVal = binData.distance;

      // Check if distance is a number before formatting
      const distance =
        typeof distanceVal === "number"
          ? `${distanceVal.toFixed(2)} cm`
          : "No data";

      const status = binData.status || "unknown";

      // Create bin card
      const card = document.createElement("div");
      card.classList.add("bin");

      card.innerHTML = `
        <h2>${name}</h2>
        <p><strong>ID:</strong> ${binID}</p>
        <p>Distance: ${distance}</p>
        <p>Status: ${status}</p>
      `;

      binsContainer.appendChild(card);
    });
  });
}
