const express = require("express");
const { WebSocketServer } = require("ws");
const { SerialPort } = require("serialport");
const { ReadlineParser } = require("@serialport/parser-readline");

const app = express();
const PORT = 3000;

// Serve files from the "public" folder
app.use(express.static("public"));

const server = app.listen(PORT, () => {
  console.log(`Server running at http://localhost:${PORT}`);
});

const wss = new WebSocketServer({ server });

const serial = new SerialPort({
  path: "/dev/ttyACM1",
  baudRate: 115200,
});

// Reads line-by-line 
const parser = serial.pipe(new ReadlineParser({ delimiter: "\r\n" }));
parser.on("data", (line) => {
  console.log("Serial:", line);

  // Send serial data to all connected browser clients
  wss.clients.forEach((client) => {
    if (client.readyState === client.OPEN) {
      client.send(line);
    }
  });
});
