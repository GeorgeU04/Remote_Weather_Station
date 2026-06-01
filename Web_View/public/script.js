async function getLatest(nodeID) {
  const response = await fetch(`/api/sensor/${nodeID}/latest`);
  const result = await response.json();

  if (!result.ok) {
    throw new Error(result.error);
  }

  return result.data;
}

async function getAllReadings(nodeID) {
  const response = await fetch(`/api/sensor/${nodeID}/all`);
  const result = await response.json();

  if (!result.ok) {
    throw new Error(result.error);
  }

  return result.data;
}

async function updateLatestDisplay(nodeID) {
  try {
    const data = await getLatest(nodeID);

    if (!data) {
      return;
    }

    document.getElementById("temperature").textContent = data.temperature;
    document.getElementById("pressure").textContent = data.pressure;
    document.getElementById("humidity").textContent = data.humidity;
    document.getElementById("timestamp").textContent = data.timeStamp;
  } catch (err) {
    console.error("Failed to update latest display:", err);
  }
}

// Update once immediately
updateLatestDisplay(0);

// Then update every 5 seconds
setInterval(() => {
  updateLatestDisplay(0);
}, 5000);
