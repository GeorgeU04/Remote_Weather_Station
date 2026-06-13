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

async function getNReadings(nodeID, n) {
  const response = await fetch(`/api/sensor/${nodeID}/last/${n}`);
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

    let temperature = data.temperature / 100;
    let pressure = data.pressure / 256;
    let humidity = data.humidity / 1024;

    document.getElementById("temperature").textContent = temperature.toFixed(2);
    document.getElementById("pressure").textContent = pressure.toFixed(2);
    document.getElementById("humidity").textContent = humidity.toFixed(2);
    document.getElementById("timeStamp").textContent = data.timeStamp;
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
