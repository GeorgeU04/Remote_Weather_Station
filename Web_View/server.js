const express = require("express");
const path = require("path");

const {
  readLast,
  readAll,
  readN,
} = require("./dbClient");

const app = express();
const PORT = 3000;

app.use(express.json());
app.use(express.static(path.join(__dirname, "public")));

app.get("/api/sensor/:nodeID/latest", async (req, res) => {
  try {
    const nodeID = Number(req.params.nodeID);

    if (!Number.isInteger(nodeID) || nodeID < 0) {
      return res.status(400).json({
        ok: false,
        error: "Invalid nodeID"
      });
    }

    const latest = await readLast(nodeID);

    res.json({
      ok: true,
      nodeID,
      data: latest
    });
  } catch (err) {
    res.status(500).json({
      ok: false,
      error: err.message
    });
  }
});

app.get("/api/sensor/:nodeID/all", async (req, res) => {
  try {
    const nodeID = Number(req.params.nodeID);

    if (!Number.isInteger(nodeID) || nodeID < 0) {
      return res.status(400).json({
        ok: false,
        error: "Invalid nodeID"
      });
    }

    const result = await readAll(nodeID);

    res.json({
      ok: true,
      nodeID,
      recordsRead: result.recordsRead,
      data: result.data
    });
  } catch (err) {
    res.status(500).json({
      ok: false,
      error: err.message
    });
  }
});

app.get("/api/sensor/:nodeID/last/:n", async (req, res) => {
  try {
    const nodeID = Number(req.params.nodeID);
    const n = Number(req.params.n);

    if (!Number.isInteger(nodeID) || nodeID < 0) {
      return res.status(400).json({
        ok: false,
        error: "Invalid nodeID"
      });
    }

    if (!Number.isInteger(n) || n <= 0) {
      return res.status(400).json({
        ok: false,
        error: "Invalid n"
      });
    }

    const result = await readN(nodeID, n);

    res.json({
      ok: true,
      nodeID,
      recordsRead: result.recordsRead,
      data: result.data
    });
  } catch (err) {
    res.status(500).json({
      ok: false,
      error: err.message
    });
  }
});

app.listen(PORT, () => {
  console.log(`Web server listening on http://localhost:${PORT}`);
});
