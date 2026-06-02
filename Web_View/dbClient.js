const net = require("net");

const DB_HOST = "127.0.0.1";
const DB_PORT = 9000;

function sendDatabaseCommand(command) {
  return new Promise((resolve, reject) => {
    const client = new net.Socket();

    let response = "";
    let finished = false;

    const cleanup = () => {
      client.removeAllListeners();
      client.destroy();
    };

    client.setTimeout(3000);

    client.connect(DB_PORT, DB_HOST, () => {
      client.write(command + "\n");
    });

    client.on("data", chunk => {
      response += chunk.toString();

      if (response.includes("\n")) {
        finished = true;

        const line = response.trim();

        cleanup();

        try {
          const parsed = JSON.parse(line);

          if (!parsed.ok) {
            reject(new Error(parsed.error || "Database command failed"));
            return;
          }

          resolve(parsed);
        } catch (err) {
          reject(new Error(`Invalid JSON from database: ${line}`));
        }
      }
    });

    client.on("timeout", () => {
      if (!finished) {
        cleanup();
        reject(new Error("Database socket timed out"));
      }
    });

    client.on("error", err => {
      if (!finished) {
        cleanup();
        reject(err);
      }
    });

    client.on("close", () => {
      if (!finished && response.length > 0) {
        try {
          const parsed = JSON.parse(response.trim());
          resolve(parsed);
        } catch {
          reject(new Error(`Socket closed with invalid response: ${response}`));
        }
      }
    });
  });
}

async function readLast(nodeID) {
  const result = await sendDatabaseCommand(`READ_LAST ${nodeID}`);

  // Expected:
  // {
  //   ok: true,
  //   recordsRead: 1,
  //   data: [{ temperature, pressure, humidity, timeStamp }]
  // }

  if (!result.data || result.data.length === 0) {
    return null;
  }

  return result.data[0];
}

async function readAll(nodeID) {
  const result = await sendDatabaseCommand(`READ_ALL ${nodeID}`);

  // Expected:
  // {
  //   ok: true,
  //   recordsRead: 5,
  //   data: [...]
  // }

  return {
    recordsRead: result.recordsRead || 0,
    data: result.data || []
  };
}

async function readN(nodeID, n) {
  const result = await sendDatabaseCommand(`READ_N ${nodeID} ${n}`);

  // Expected:
  // {
  //   ok: true,
  //   recordsRead: 5,
  //   data: [...]
  // }

  return {
    recordsRead: result.recordsRead || 0,
    data: result.data || []
  };
}

module.exports = {
  sendDatabaseCommand,
  readLast,
  readAll,
  readN,
};
