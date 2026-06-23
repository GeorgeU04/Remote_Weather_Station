const NODE_ID = 0;
const REFRESH_MS = 5000;
const HPA_PER_INHG = 33.8638866667;

const els = {
  temperature: document.getElementById("temperature"),
  tempUnit: document.getElementById("tempUnit"),
  humidity: document.getElementById("humidity"),
  pressure: document.getElementById("pressure"),
  pressureUnit: document.getElementById("pressureUnit"),
  dewPoint: document.getElementById("dewPoint"),
  dewPointUnit: document.getElementById("dewPointUnit"),
  feelsLike: document.getElementById("feelsLike"),
  feelsLikeUnit: document.getElementById("feelsLikeUnit"),
  timeStamp: document.getElementById("timeStamp"),
  conditions: document.getElementById("conditions"),
  weatherIcon: document.getElementById("weatherIcon"),
  lastUpdated: document.getElementById("lastUpdated"),
  connectionStatus: document.getElementById("connectionStatus"),
  liveDot: document.getElementById("liveDot"),
  readAll: document.getElementById("readAll"),
  readN: document.getElementById("readN"),
  nInput: document.getElementById("nInput"),
  chartEmpty: document.getElementById("chartEmpty"),
  recordsInfo: document.getElementById("recordsInfo"),
  toast: document.getElementById("toast"),
  tempUnitC: document.getElementById("tempUnitC"),
  tempUnitF: document.getElementById("tempUnitF"),
  pressureUnitHpa: document.getElementById("pressureUnitHpa"),
  pressureUnitInhg: document.getElementById("pressureUnitInhg"),
};

const units = {
  temp: localStorage.getItem("tempUnit") === "F" ? "F" : "C",
  pressure: localStorage.getItem("pressureUnit") === "inhg" ? "inhg" : "hpa",
};

let trendChart = null;
let toastTimer = null;
let lastLiveReading = null;
let lastChartReadings = null;

function decodeReading(raw) {
  return {
    temperature: raw.temperature / 100,
    pressurePa: raw.pressure / 256,
    humidity: raw.humidity / 1024,
    timeStamp: raw.timeStamp,
  };
}

function paToHpa(pa) {
  return pa / 100;
}

function hpaToInHg(hpa) {
  return hpa / HPA_PER_INHG;
}

function cToF(c) {
  return c * 9 / 5 + 32;
}

function formatTempC(c) {
  if (units.temp === "F") {
    return cToF(c);
  }
  return c;
}

function tempUnitLabel() {
  return units.temp === "F" ? "\u00B0F" : "\u00B0C";
}

function formatPressureHpa(hpa) {
  if (units.pressure === "inhg") {
    return hpaToInHg(hpa);
  }
  return hpa;
}

function pressureUnitLabel() {
  return units.pressure === "inhg" ? "inHg" : "hPa";
}

function isUnixTimestamp(timeStamp) {
  return Number.isFinite(timeStamp) && timeStamp > 1e9;
}

function timestampToDate(timeStamp, anchorTs) {
  if (isUnixTimestamp(timeStamp)) {
    return new Date(timeStamp * 1000);
  }

  const anchor = anchorTs ?? timeStamp;
  const deltaSec = anchor - timeStamp;
  return new Date(Date.now() - deltaSec * 1000);
}

function formatReadingTime(timeStamp, anchorTs) {
  const date = timestampToDate(timeStamp, anchorTs);
  return date.toLocaleString([], {
    month: "short",
    day: "numeric",
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
  });
}

function formatChartTime(timeStamp, anchorTs) {
  const date = timestampToDate(timeStamp, anchorTs);
  const sameDay = new Date().toDateString() === date.toDateString();

  if (sameDay) {
    return date.toLocaleTimeString([], {
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit",
    });
  }

  return date.toLocaleString([], {
    month: "short",
    day: "numeric",
    hour: "2-digit",
    minute: "2-digit",
  });
}

function calcDewPointC(tempC, rh) {
  const a = 17.27;
  const b = 237.7;
  const alpha = (a * tempC) / (b + tempC) + Math.log(rh / 100);
  return (b * alpha) / (a - alpha);
}

function calcHeatIndexC(tempC, rh) {
  const tempF = cToF(tempC);
  if (tempF < 80) {
    return tempC;
  }

  let hi =
    -42.379 +
    2.04901523 * tempF +
    10.14333127 * rh -
    0.22475541 * tempF * rh -
    0.00683783 * tempF * tempF -
    0.05481717 * rh * rh +
    0.00122874 * tempF * tempF * rh +
    0.00085282 * tempF * rh * rh -
    0.00000199 * tempF * tempF * rh * rh;

  return (hi - 32) * 5 / 9;
}

function describeConditions(tempC, rh) {
  if (tempC < 0) return "Freezing";
  if (tempC < 10 && rh > 80) return "Cold & damp";
  if (tempC < 10) return "Cold";
  if (tempC < 18 && rh > 75) return "Cool & humid";
  if (tempC < 18) return "Cool";
  if (tempC < 24 && rh > 70) return "Mild & humid";
  if (tempC < 24) return "Pleasant";
  if (tempC < 30 && rh > 65) return "Warm & muggy";
  if (tempC < 30) return "Warm";
  if (rh > 60) return "Hot & humid";
  return "Hot & dry";
}

function weatherIcon(tempC, rh) {
  if (tempC < 0) return "\u2744\uFE0F";
  if (rh > 85) return "\uD83C\uDF27\uFE0F";
  if (rh > 65) return "\u2601\uFE0F";
  if (tempC > 28) return "\u2600\uFE0F";
  if (tempC > 18) return "\u26C5";
  return "\uD83C\uDF24\uFE0F";
}

function setTempTheme(tempC) {
  document.body.classList.remove("temp-cold", "temp-cool", "temp-mild", "temp-warm", "temp-hot");
  if (tempC < 5) document.body.classList.add("temp-cold");
  else if (tempC < 15) document.body.classList.add("temp-cool");
  else if (tempC < 22) document.body.classList.add("temp-mild");
  else if (tempC < 30) document.body.classList.add("temp-warm");
  else document.body.classList.add("temp-hot");
}

function formatTime(date) {
  return date.toLocaleTimeString([], { hour: "2-digit", minute: "2-digit", second: "2-digit" });
}

function updateUnitButtons() {
  els.tempUnitC.classList.toggle("active", units.temp === "C");
  els.tempUnitF.classList.toggle("active", units.temp === "F");
  els.pressureUnitHpa.classList.toggle("active", units.pressure === "hpa");
  els.pressureUnitInhg.classList.toggle("active", units.pressure === "inhg");
}

function setTempUnit(unit) {
  units.temp = unit;
  localStorage.setItem("tempUnit", unit);
  updateUnitButtons();
  if (lastLiveReading) {
    updateCurrentDisplay(lastLiveReading);
  }
  if (lastChartReadings) {
    buildChart(lastChartReadings);
  }
}

function setPressureUnit(unit) {
  units.pressure = unit;
  localStorage.setItem("pressureUnit", unit);
  updateUnitButtons();
  if (lastLiveReading) {
    updateCurrentDisplay(lastLiveReading);
  }
  if (lastChartReadings) {
    buildChart(lastChartReadings);
  }
}

function showToast(message) {
  els.toast.textContent = message;
  els.toast.classList.add("visible");
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => els.toast.classList.remove("visible"), 4000);
}

function setOnline(online) {
  els.liveDot.classList.toggle("offline", !online);
  els.connectionStatus.textContent = online ? "Live" : "Offline";
}

async function getLatest(nodeID) {
  const response = await fetch(`/api/sensor/${nodeID}/latest`);
  const result = await response.json();
  if (!result.ok) throw new Error(result.error);
  return result.data;
}

async function getAllReadings(nodeID) {
  const response = await fetch(`/api/sensor/${nodeID}/all`);
  const result = await response.json();
  if (!result.ok) throw new Error(result.error);
  return result;
}

async function getNReadings(nodeID, n) {
  const response = await fetch(`/api/sensor/${nodeID}/last/${n}`);
  const result = await response.json();
  if (!result.ok) throw new Error(result.error);
  return result;
}

function updateCurrentDisplay(reading) {
  const { temperature, pressurePa, humidity, timeStamp } = reading;
  const pressureHpa = paToHpa(pressurePa);
  const dew = calcDewPointC(temperature, humidity);
  const feels = calcHeatIndexC(temperature, humidity);
  const tempLabel = tempUnitLabel();
  const pressureLabel = pressureUnitLabel();

  els.temperature.textContent = formatTempC(temperature).toFixed(1);
  els.tempUnit.textContent = tempLabel;
  els.humidity.textContent = humidity.toFixed(1);
  els.pressure.textContent = formatPressureHpa(pressureHpa).toFixed(units.pressure === "inhg" ? 2 : 1);
  els.pressureUnit.textContent = pressureLabel;
  els.dewPoint.textContent = formatTempC(dew).toFixed(1);
  els.dewPointUnit.textContent = tempLabel;
  els.feelsLike.textContent = formatTempC(feels).toFixed(1);
  els.feelsLikeUnit.textContent = tempLabel;
  els.timeStamp.textContent = formatReadingTime(timeStamp, timeStamp);
  els.conditions.textContent = describeConditions(temperature, humidity);
  els.weatherIcon.textContent = weatherIcon(temperature, humidity);

  setTempTheme(temperature);
  els.lastUpdated.textContent = `Updated ${formatTime(new Date())}`;
  setOnline(true);
}

async function updateLatestDisplay(nodeID) {
  try {
    const data = await getLatest(nodeID);
    if (!data) {
      lastLiveReading = null;
      els.conditions.textContent = "No readings yet";
      setOnline(false);
      return;
    }
    lastLiveReading = decodeReading(data);
    updateCurrentDisplay(lastLiveReading);
  } catch (err) {
    console.error("Failed to update latest display:", err);
    setOnline(false);
    els.connectionStatus.textContent = "Error";
  }
}

function buildChart(readings) {
  const decoded = readings.map(decodeReading).reverse();
  const anchorTs = decoded.reduce((max, r) => Math.max(max, r.timeStamp), decoded[0]?.timeStamp ?? 0);
  const labels = decoded.map((r) => formatChartTime(r.timeStamp, anchorTs));
  const temps = decoded.map((r) => formatTempC(r.temperature));
  const humidities = decoded.map((r) => r.humidity);
  const pressures = decoded.map((r) => formatPressureHpa(paToHpa(r.pressurePa)));
  const tempLabel = tempUnitLabel();
  const pressureLabel = pressureUnitLabel();

  els.chartEmpty.classList.add("hidden");

  const ctx = document.getElementById("trendChart").getContext("2d");

  if (trendChart) {
    trendChart.destroy();
  }

  trendChart = new Chart(ctx, {
    type: "line",
    data: {
      labels,
      datasets: [
        {
          label: `Temperature (${tempLabel})`,
          data: temps,
          borderColor: "#ffb347",
          backgroundColor: "rgba(255, 179, 71, 0.12)",
          yAxisID: "y",
          tension: 0.35,
          pointRadius: readings.length > 40 ? 0 : 3,
          fill: true,
        },
        {
          label: "Humidity (%)",
          data: humidities,
          borderColor: "#4db8ff",
          backgroundColor: "rgba(77, 184, 255, 0.08)",
          yAxisID: "y1",
          tension: 0.35,
          pointRadius: readings.length > 40 ? 0 : 3,
        },
        {
          label: `Pressure (${pressureLabel})`,
          data: pressures,
          borderColor: "#3dd68c",
          backgroundColor: "rgba(61, 214, 140, 0.08)",
          yAxisID: "y2",
          tension: 0.35,
          pointRadius: readings.length > 40 ? 0 : 3,
        },
      ],
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      interaction: { mode: "index", intersect: false },
      plugins: {
        legend: {
          labels: { color: "rgba(244, 247, 251, 0.8)", boxWidth: 12, padding: 16 },
        },
        tooltip: {
          backgroundColor: "rgba(10, 22, 40, 0.92)",
          titleColor: "#f4f7fb",
          bodyColor: "rgba(244, 247, 251, 0.85)",
          borderColor: "rgba(255, 255, 255, 0.14)",
          borderWidth: 1,
        },
      },
      scales: {
        x: {
          ticks: { color: "rgba(244, 247, 251, 0.45)", maxTicksLimit: 10 },
          grid: { color: "rgba(255, 255, 255, 0.06)" },
        },
        y: {
          type: "linear",
          position: "left",
          title: { display: true, text: tempLabel, color: "rgba(255, 179, 71, 0.8)" },
          ticks: { color: "rgba(255, 179, 71, 0.7)" },
          grid: { color: "rgba(255, 255, 255, 0.06)" },
        },
        y1: {
          type: "linear",
          position: "right",
          title: { display: true, text: "% RH", color: "rgba(77, 184, 255, 0.8)" },
          ticks: { color: "rgba(77, 184, 255, 0.7)" },
          grid: { drawOnChartArea: false },
          min: 0,
          max: 100,
        },
        y2: {
          type: "linear",
          position: "right",
          title: { display: true, text: pressureLabel, color: "rgba(61, 214, 140, 0.8)" },
          ticks: { color: "rgba(61, 214, 140, 0.7)", display: false },
          grid: { drawOnChartArea: false },
        },
      },
    },
  });
}

async function loadHistory(fetchFn) {
  const buttons = [els.readAll, els.readN];
  buttons.forEach((b) => (b.disabled = true));

  try {
    const result = await fetchFn();
    const readings = result.data || [];

    if (readings.length === 0) {
      lastChartReadings = null;
      els.chartEmpty.classList.remove("hidden");
      els.chartEmpty.textContent = "No historical readings found";
      els.recordsInfo.textContent = "";
      if (trendChart) {
        trendChart.destroy();
        trendChart = null;
      }
      return;
    }

    lastChartReadings = readings;
    buildChart(readings);
    els.recordsInfo.textContent = `Showing ${result.recordsRead ?? readings.length} reading(s)`;
  } catch (err) {
    console.error("Failed to load history:", err);
    showToast(err.message || "Failed to load history");
  } finally {
    buttons.forEach((b) => (b.disabled = false));
  }
}

els.readAll.addEventListener("click", () => {
  loadHistory(() => getAllReadings(NODE_ID));
});

els.readN.addEventListener("click", () => {
  const n = parseInt(els.nInput.value, 10);
  if (!Number.isInteger(n) || n <= 0) {
    showToast("Enter a valid number of readings");
    return;
  }
  loadHistory(() => getNReadings(NODE_ID, n));
});

els.tempUnitC.addEventListener("click", () => setTempUnit("C"));
els.tempUnitF.addEventListener("click", () => setTempUnit("F"));
els.pressureUnitHpa.addEventListener("click", () => setPressureUnit("hpa"));
els.pressureUnitInhg.addEventListener("click", () => setPressureUnit("inhg"));

updateUnitButtons();
updateLatestDisplay(NODE_ID);
setInterval(() => updateLatestDisplay(NODE_ID), REFRESH_MS);

loadHistory(() => getNReadings(NODE_ID, parseInt(els.nInput.value, 10) || 20));
