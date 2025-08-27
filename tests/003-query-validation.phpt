--TEST--
MySQL query validation functionality
--SKIPIF--
<?php if (!extension_loaded("mysql_qp")) print "skip"; ?>
--FILE--
<?php
// Test valid queries
$valid_queries = [
    "SELECT * FROM users",
    "INSERT INTO users VALUES (1, 'test')",
    "UPDATE users SET name = 'test'",
    "DELETE FROM users WHERE id = 1",
    "SHOW TABLES"
];

foreach ($valid_queries as $query) {
    $valid = mysql_validate_query($query);
    echo "'" . substr($query, 0, 15) . "...' is " . ($valid ? "valid" : "invalid") . "\n";
}

// Test invalid queries
$invalid_queries = [
    "SELECTT * FROM users",
    "SELECT * FROM",
    "INSERT VALUES",
    "INVALID SYNTAX"
];

foreach ($invalid_queries as $query) {
    $valid = mysql_validate_query($query);
    echo "'" . $query . "' is " . ($valid ? "valid" : "invalid") . "\n";
}
?>
--EXPECT--
'SELECT * FROM u...' is valid
'INSERT INTO use...' is valid
'UPDATE users SE...' is valid
'DELETE FROM use...' is valid
'SHOW TABLES...' is valid
'SELECTT * FROM users' is invalid
'SELECT * FROM' is invalid
'INSERT VALUES' is invalid
'INVALID SYNTAX' is invalid