const net = require("net");

const DB_HOST = "127.0.0.1";
const DB_PORT = 9000;

// The database server handles one TCP client at a time. Serialize commands here
// so concurrent HTTP requests wait their turn instead of racing to connect.
let commandTail = Promise.resolve();

function sendDatabaseCommandOnce(command) {
  return new Promise((resolve, reject) => {
    const client = new net.Socket();

    let response = "";
    let finished = false;

    const cleanup = () => {
      client.removeAllListeners();
      client.destroy();
    };

    const finish = (fn) => {
      if (finished) {
        return;
      }
      finished = true;
      cleanup();
      fn();
    };

    client.setTimeout(15000);

    client.connect(DB_PORT, DB_HOST, () => {
      client.write(command + "\n");
    });

    client.on("data", chunk => {
      response += chunk.toString();

      if (response.includes("\n")) {
        const line = response.trim();

        try {
          const parsed = JSON.parse(line);

          if (!parsed.ok) {
            finish(() => reject(new Error(parsed.error || "Database command failed")));
            return;
          }

          finish(() => resolve(parsed));
        } catch (err) {
          finish(() => reject(new Error(`Invalid JSON from database: ${line}`)));
        }
      }
    });

    client.on("timeout", () => {
      finish(() => reject(new Error("Database socket timed out")));
    });

    client.on("error", err => {
      finish(() => reject(err));
    });

    client.on("close", () => {
      if (finished) {
        return;
      }

      if (response.length > 0) {
        try {
          const parsed = JSON.parse(response.trim());

          if (!parsed.ok) {
            finish(() => reject(new Error(parsed.error || "Database command failed")));
            return;
          }

          finish(() => resolve(parsed));
        } catch {
          finish(() => reject(new Error(`Socket closed with invalid response: ${response}`)));
        }
        return;
      }

      finish(() => reject(new Error("Database connection closed before response")));
    });
  });
}

function sendDatabaseCommand(command) {
  const result = commandTail.then(() => sendDatabaseCommandOnce(command));
  commandTail = result.catch(() => {});
  return result;
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
