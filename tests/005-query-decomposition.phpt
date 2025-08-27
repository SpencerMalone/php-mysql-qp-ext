--TEST--
Query decomposition and reconstruction
--SKIPIF--
<?php if (!extension_loaded("mysql_qp")) print "skip"; ?>
--FILE--
<?php
// Test basic SELECT decomposition
$components = mysql_decompose_query("SELECT id, name FROM users WHERE age > 18");
echo "Query type: " . $components['type'] . "\n";
echo "Field count: " . count($components['fields']) . "\n";
echo "First field: " . $components['fields'][0] . "\n";
echo "Table name: " . $components['tables'][0]['table'] . "\n";
echo "Has WHERE: " . (count($components['where_conditions']) > 0 ? "YES" : "NO") . "\n";

// Test reconstruction
$rebuilt = mysql_reconstruct_query($components);
echo "Rebuilt valid: " . (mysql_validate_query($rebuilt) ? "YES" : "NO") . "\n";

// Test with ORDER BY and LIMIT
$components2 = mysql_decompose_query("SELECT * FROM users ORDER BY name LIMIT 5");
echo "Has ORDER BY: " . (count($components2['order_by']) > 0 ? "YES" : "NO") . "\n";
echo "Has LIMIT: " . (count($components2['limit_clause']) > 0 ? "YES" : "NO") . "\n";

$rebuilt2 = mysql_reconstruct_query($components2);
echo "Complex rebuilt valid: " . (mysql_validate_query($rebuilt2) ? "YES" : "NO") . "\n";
?>
--EXPECT--
Query type: SELECT
Field count: 2
First field: id
Table name: users
Has WHERE: YES
Rebuilt valid: YES
Has ORDER BY: YES
Has LIMIT: YES
Complex rebuilt valid: YES