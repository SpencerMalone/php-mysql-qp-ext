--TEST--
MySQL query parsing functionality
--SKIPIF--
<?php if (!extension_loaded("mysql_qp")) print "skip"; ?>
--FILE--
<?php
// Test valid SELECT query
$result = mysql_parse_query("SELECT * FROM users WHERE id = ?");
echo "SELECT query valid: " . ($result['is_valid'] ? "YES" : "NO") . "\n";
echo "Query type: " . $result['query_type'] . "\n";
echo "Parameter count: " . $result['parameter_count'] . "\n";

// Test valid INSERT query
$result = mysql_parse_query("INSERT INTO users (name, email) VALUES (?, ?)");
echo "INSERT query valid: " . ($result['is_valid'] ? "YES" : "NO") . "\n";
echo "Query type: " . $result['query_type'] . "\n";
echo "Parameter count: " . $result['parameter_count'] . "\n";

// Test invalid query
$result = mysql_parse_query("SELECTT * FROM users");
echo "Invalid query valid: " . ($result['is_valid'] ? "YES" : "NO") . "\n";
echo "Error code: " . $result['error_code'] . "\n";
?>
--EXPECT--
SELECT query valid: YES
Query type: 1
Parameter count: 1
INSERT query valid: YES
Query type: 2
Parameter count: 2
Invalid query valid: NO
Error code: 1064