--TEST--
Basic extension loading
--SKIPIF--
<?php if (!extension_loaded("mysql_qp")) print "skip"; ?>
--FILE--
<?php
echo "mysql_qp extension is loaded\n";
var_dump(extension_loaded('mysql_qp'));
?>
--EXPECT--
mysql_qp extension is loaded
bool(true)