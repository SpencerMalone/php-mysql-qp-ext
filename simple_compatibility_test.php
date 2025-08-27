<?php
/**
 * Simple MySQL Version Compatibility Test
 * Focus on syntax validation to avoid memory issues
 */

if (!extension_loaded('mysql_qp')) {
    echo "❌ mysql_qp extension not loaded\n";
    exit(1);
}

echo "MySQL Version Compatibility Test (Simple)\n";
echo "==========================================\n\n";

// Test queries covering different MySQL features across versions
$compatibility_tests = [
    // Basic SQL (MySQL 5.x+)
    [
        'version' => '5.x+',
        'feature' => 'Basic SELECT',
        'query' => 'SELECT id, name FROM users WHERE status = "active"'
    ],
    [
        'version' => '5.x+', 
        'feature' => 'Basic INSERT',
        'query' => 'INSERT INTO users (name, email) VALUES ("John", "john@example.com")'
    ],
    [
        'version' => '5.x+',
        'feature' => 'Basic UPDATE', 
        'query' => 'UPDATE users SET status = "active" WHERE id = 1'
    ],
    [
        'version' => '5.x+',
        'feature' => 'Basic DELETE',
        'query' => 'DELETE FROM users WHERE status = "inactive"'
    ],

    // Advanced SQL features (MySQL 5.6+)
    [
        'version' => '5.6+',
        'feature' => 'Simple Subquery',
        'query' => 'SELECT * FROM users WHERE id IN (SELECT user_id FROM orders)'
    ],
    [
        'version' => '5.6+', 
        'feature' => 'Window Functions Syntax',
        'query' => 'SELECT name, ROW_NUMBER() OVER (ORDER BY id) as row_num FROM users'
    ],

    // MySQL 5.7+ features
    [
        'version' => '5.7+',
        'feature' => 'JSON Functions',
        'query' => 'SELECT JSON_EXTRACT(data, "$.type") as user_type FROM users'
    ],
    [
        'version' => '5.7+',
        'feature' => 'Generated Columns',
        'query' => 'ALTER TABLE users ADD COLUMN full_name VARCHAR(100) GENERATED ALWAYS AS (CONCAT(first_name, " ", last_name))'
    ],

    // MySQL 8.0+ features  
    [
        'version' => '8.0+',
        'feature' => 'Common Table Expressions',
        'query' => 'WITH user_stats AS (SELECT id, name FROM users) SELECT * FROM user_stats'
    ],
    [
        'version' => '8.0+',
        'feature' => 'Window Functions Advanced',
        'query' => 'SELECT name, RANK() OVER (PARTITION BY department ORDER BY salary DESC) as rank FROM employees'
    ],
    [
        'version' => '8.0+', 
        'feature' => 'Roles Syntax',
        'query' => 'CREATE ROLE app_read, app_write'
    ],

    // MySQL 8.4+ features (latest)
    [
        'version' => '8.4+',
        'feature' => 'Multi-Value Index Syntax', 
        'query' => 'CREATE INDEX idx_tags ON articles((CAST(tags->"$[*]" AS CHAR(50) ARRAY)))'
    ],
];

echo "Current Environment:\n";
echo "===================\n";

// Test basic extension functionality
try {
    $parse_result = mysql_parse_query("SELECT VERSION()");
    if ($parse_result['is_valid']) {
        echo "✅ Extension can parse VERSION() query\n";
    }
} catch (Exception $e) {
    echo "⚠️  Could not test VERSION() query: " . $e->getMessage() . "\n";
}

echo "\nCompatibility Test Results (Syntax-Only):\n";
echo "==========================================\n";
printf("%-8s %-30s %-12s %s\n", 
       "Version", "Feature", "Syntax", "Notes");
echo str_repeat("-", 70) . "\n";

$results = [];
$total_tests = count($compatibility_tests);
$passed_tests = 0;

foreach ($compatibility_tests as $test) {
    $feature = substr($test['feature'], 0, 28);
    $query = $test['query'];
    
    // Test syntax validation only (safe)
    $syntax_valid = mysql_validate_query($query);
    $syntax_ok = $syntax_valid ? "✅ PASS" : "❌ FAIL";
    
    // Notes
    $notes = $syntax_valid ? "OK" : "Syntax error";
    
    // Track results
    if ($syntax_valid) {
        $passed_tests++;
        $results[$test['version']][] = ['feature' => $test['feature'], 'status' => 'PASS'];
    } else {
        $results[$test['version']][] = ['feature' => $test['feature'], 'status' => 'FAIL'];
    }
    
    printf("%-8s %-30s %-12s %s\n", 
           $test['version'], $feature, $syntax_ok, $notes);
}

// Summary by version
echo "\n" . str_repeat("-", 70) . "\n";
echo "Summary by MySQL Version:\n";
echo "========================\n";

$version_summary = [];
foreach ($results as $version => $version_tests) {
    $total = count($version_tests);
    $passed = count(array_filter($version_tests, fn($t) => $t['status'] === 'PASS'));
    $percentage = $total > 0 ? round(($passed / $total) * 100, 1) : 0;
    
    $version_summary[$version] = [
        'total' => $total,
        'passed' => $passed, 
        'percentage' => $percentage
    ];
    
    $status_icon = $percentage >= 90 ? "✅" : ($percentage >= 70 ? "⚠️" : "❌");
    echo sprintf("%s MySQL %s: %d/%d tests passed (%.1f%%)\n", 
                 $status_icon, $version, $passed, $total, $percentage);
}

echo "\nOverall Compatibility: {$passed_tests}/{$total_tests} tests passed (" . 
     round(($passed_tests / $total_tests) * 100, 1) . "%)\n";

// Recommendations
echo "\nRecommendations:\n";
echo "================\n";

if ($passed_tests / $total_tests >= 0.9) {
    echo "✅ Excellent compatibility! Extension works well across MySQL versions.\n";
} elseif ($passed_tests / $total_tests >= 0.7) {
    echo "⚠️  Good compatibility with some limitations on newer features.\n";
} else {
    echo "❌ Limited compatibility. Consider testing with different MySQL client libraries.\n";
}

// Compatibility matrix
echo "\nCompatibility Matrix:\n";
echo "====================\n";
echo "| MySQL Version | Features Supported | Compatibility |\n";
echo "|---------------|-------------------|---------------|\n";
foreach ($version_summary as $version => $summary) {
    $compat = $summary['percentage'] >= 90 ? "Full" : ($summary['percentage'] >= 70 ? "Partial" : "Limited");
    echo sprintf("| %-13s | %-17s | %-13s |\n", 
                 $version, "{$summary['passed']}/{$summary['total']}", $compat);
}

echo "\nTest completed successfully!\n";
?>