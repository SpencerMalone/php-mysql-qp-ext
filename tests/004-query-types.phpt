--TEST--
MySQL query type detection
--SKIPIF--
<?php if (!extension_loaded("mysql_qp")) print "skip"; ?>
--FILE--
<?php
$test_cases = [
    ["SELECT * FROM users", 1, "SELECT"],
    ["INSERT INTO users VALUES (1)", 2, "INSERT"], 
    ["UPDATE users SET name = 'test'", 3, "UPDATE"],
    ["DELETE FROM users", 4, "DELETE"],
    ["CREATE TABLE test (id INT)", 5, "CREATE"],
    ["DROP TABLE test", 6, "DROP"],
    ["ALTER TABLE users ADD COLUMN age INT", 7, "ALTER"],
    ["SHOW TABLES", 8, "SHOW"],
    ["DESCRIBE users", 9, "DESCRIBE"],
    ["EXPLAIN SELECT * FROM users", 10, "EXPLAIN"]
];

foreach ($test_cases as [$query, $expected_type, $type_name]) {
    $result = mysql_parse_query($query);
    $actual_type = $result['query_type'];
    echo "$type_name query type: " . ($actual_type == $expected_type ? "CORRECT ($actual_type)" : "WRONG (got $actual_type, expected $expected_type)") . "\n";
}
?>
--EXPECT--
SELECT query type: CORRECT (1)
INSERT query type: CORRECT (2)
UPDATE query type: CORRECT (3)
DELETE query type: CORRECT (4)
CREATE query type: CORRECT (5)
DROP query type: CORRECT (6)
ALTER query type: CORRECT (7)
SHOW query type: CORRECT (8)
DESCRIBE query type: CORRECT (9)
EXPLAIN query type: CORRECT (10)