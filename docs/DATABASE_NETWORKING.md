# Database & Networking Features

Axolotl supports optional MySQL database and WebSocket connectivity.

## Installation

### macOS
```bash
# MySQL support
brew install mysql-connector-c

# WebSocket support
brew install libwebsockets
```

### Linux (Ubuntu/Debian)
```bash
# MySQL support
sudo apt-get install libmysqlclient-dev

# WebSocket support
sudo apt-get install libwebsockets-dev
```

### Linux (Fedora/RHEL)
```bash
# MySQL support
sudo dnf install mysql-devel

# WebSocket support
sudo dnf install libwebsockets-devel
```

After installing the libraries, rebuild Axolotl:
```bash
make clean
make build
```

## MySQL Database

### Connect to Database
```axolotl
const db: object = mysqlConnect("localhost", "username", "password", "database");
```

### Execute Queries
```axolotl
// CREATE
db.query("CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR(255))");

// INSERT
db.query("INSERT INTO users VALUES (1, 'Alice')");

// SELECT (returns array of objects)
const users: any = db.query("SELECT * FROM users");
var i: int = 0;
while (i < len(users)) {
    print(users[i].name);
    i = i + 1;
}

// UPDATE
db.query("UPDATE users SET name = 'Bob' WHERE id = 1");

// DELETE
db.query("DELETE FROM users WHERE id = 1");
```

### Close Connection
```axolotl
db.close();
```

## WebSocket

### Connect to Server
```axolotl
// ws:// or wss:// protocols supported
const ws: object = wsConnect("ws://localhost:8080/chat");
```

### Send Messages
```axolotl
ws.send("Hello, server!");
```

### Receive Messages
```axolotl
// Non-blocking - returns empty string if no message
const message: string = ws.receive();
if (message != "") {
    print("Received: " + message);
}
```

### Close Connection
```axolotl
ws.close();
```

## Examples

See:
- `examples/mysql_example.axo` - Database CRUD operations
- `examples/websocket_example.axo` - Real-time messaging

## API Reference

### MySQL Functions

- **mysqlConnect(host: string, user: string, password: string, database: string) -> object**
  - Connects to MySQL database
  - Returns connection object

- **conn.query(sql: string) -> any**
  - Executes SQL query
  - Returns array of objects for SELECT
  - Returns affected row count for INSERT/UPDATE/DELETE

- **conn.close() -> void**
  - Closes database connection

### WebSocket Functions

- **wsConnect(url: string) -> object**
  - Connects to WebSocket server
  - Supports ws:// and wss:// protocols
  - Returns WebSocket object

- **ws.send(message: string) -> void**
  - Sends text message to server

- **ws.receive() -> string**
  - Receives message from server (non-blocking)
  - Returns empty string if no message available

- **ws.close() -> void**
  - Closes WebSocket connection

## Notes

- These features are optional and only available if the libraries are installed
- The compiler will build without them if libraries are not found
- Check build output for "Found MySQL" or "Found libwebsockets" messages
