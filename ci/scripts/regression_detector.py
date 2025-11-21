#!/usr/bin/env python3
"""
Regression Detector: Compares performance metrics against historical baseline
Database: SQLite with schema (test_name, commit_hash, timestamp, metrics_json)
Threshold: Fail if any metric degrades >5% vs last 5 commits' median
"""

import sqlite3
import json
import sys
import argparse
import statistics
import time
from pathlib import Path

DB_SCHEMA = """
CREATE TABLE IF NOT EXISTS benchmarks (
    id INTEGER PRIMARY KEY,
    test_name TEXT NOT NULL,
    commit_hash TEXT NOT NULL,
    timestamp REAL NOT NULL,
    metrics_json TEXT NOT NULL,
    UNIQUE(test_name, commit_hash)
);

CREATE TABLE IF NOT EXISTS metrics (
    benchmark_id INTEGER,
    metric_name TEXT NOT NULL,
    value REAL NOT NULL,
    unit TEXT,
    FOREIGN KEY (benchmark_id) REFERENCES benchmarks(id)
);
"""

class RegressionDetector:
    def __init__(self, db_path):
        self.conn = sqlite3.connect(db_path)
        self.cursor = self.conn.cursor()
        self.cursor.executescript(DB_SCHEMA)
        self.conn.commit()
    
    def add_measurement(self, test_name, commit_hash, metrics):
        """Add new benchmark measurement"""
        timestamp = time.time()
        metrics_json = json.dumps(metrics)
        
        self.cursor.execute(
            "INSERT OR REPLACE INTO benchmarks (test_name, commit_hash, timestamp, metrics_json) VALUES (?, ?, ?, ?)",
            (test_name, commit_hash, timestamp, metrics_json)
        )
        benchmark_id = self.cursor.lastrowid
        
        # Store individual metrics
        for metric_name, value in metrics.items():
            if isinstance(value, (int, float)):
                self.cursor.execute(
                    "INSERT INTO metrics (benchmark_id, metric_name, value) VALUES (?, ?, ?)",
                    (benchmark_id, metric_name, float(value))
                )
        self.conn.commit()
    
    def get_baseline(self, test_name, metric_name, window=5):
        """Get median baseline from last N commits"""
        self.cursor.execute("""
            SELECT value FROM metrics
            JOIN benchmarks ON metrics.benchmark_id = benchmarks.id
            WHERE test_name = ? AND metric_name = ?
            ORDER BY timestamp DESC
            LIMIT ?
        """, (test_name, metric_name, window))
        
        values = [row[0] for row in self.cursor.fetchall()]
        if not values:
            return None
        return statistics.median(values)
    
    def check_regression(self, test_name, metrics, threshold=0.05):
        """Check for regressions against baseline"""
        regressions = []
        
        for metric_name, new_value in metrics.items():
            if not isinstance(new_value, (int, float)):
                continue
                
            baseline = self.get_baseline(test_name, metric_name)
            if baseline is None:
                continue  # No baseline, skip
            
            # For metrics where lower is better (time, latency)
            # regression is when new_value > baseline
            if new_value > baseline * (1 + threshold):
                regression = (new_value - baseline) / baseline
                regressions.append({
                    'test': test_name,
                    'metric': metric_name,
                    'baseline': baseline,
                    'new': new_value,
                    'regression_pct': regression * 100
                })
        
        return regressions
    
    def generate_report(self, regressions, output_file):
        """Generate human-readable report"""
        with open(output_file, 'w') as f:
            if not regressions:
                f.write("# ✅ No Performance Regressions Detected\n")
                return
            
            f.write("# ❌ Performance Regressions Detected\n\n")
            f.write("| Test | Metric | Baseline | New | Regression |\n")
            f.write("|------|--------|----------|-----|------------|\n")
            
            for reg in regressions:
                f.write(f"| {reg['test']} | {reg['metric']} | {reg['baseline']:.3f} | "
                       f"{reg['new']:.3f} | +{reg['regression_pct']:.1f}% |\n")
            
            f.write("\n**REGRESSION**")  # Marker for CI to fail
    
    def close(self):
        self.conn.close()

def main():
    parser = argparse.ArgumentParser(description='Detect performance regressions')
    parser.add_argument("--baseline", required=True, help="Baseline database path")
    parser.add_argument("--new", required=True, help="New benchmark JSON files (glob pattern)")
    parser.add_argument("--threshold", type=float, default=0.05, help="Regression threshold (default: 0.05)")
    parser.add_argument("--output", required=True, help="Output report path")
    parser.add_argument("--commit", default="HEAD", help="Git commit hash")
    
    args = parser.parse_args()
    
    detector = RegressionDetector(args.baseline)
    
    # Load new measurements
    all_regressions = []
    new_files = list(Path(args.new).parent.glob(Path(args.new).name))
    
    if not new_files:
        print(f"Warning: No benchmark files found matching {args.new}")
        detector.close()
        return 0
    
    for json_file in new_files:
        try:
            with open(json_file) as f:
                data = json.load(f)
            
            # Handle Google Benchmark JSON format
            if 'benchmarks' in data:
                for benchmark in data['benchmarks']:
                    test_name = benchmark['name']
                    metrics = {
                        'real_time': benchmark.get('real_time', 0),
                        'cpu_time': benchmark.get('cpu_time', 0),
                        'iterations': benchmark.get('iterations', 0)
                    }
                    
                    detector.add_measurement(test_name, args.commit, metrics)
                    regressions = detector.check_regression(test_name, metrics, args.threshold)
                    all_regressions.extend(regressions)
            else:
                # Custom format
                test_name = data.get('test_name', json_file.stem)
                metrics = data.get('metrics', {})
                
                detector.add_measurement(test_name, args.commit, metrics)
                regressions = detector.check_regression(test_name, metrics, args.threshold)
                all_regressions.extend(regressions)
                
        except Exception as e:
            print(f"Error processing {json_file}: {e}", file=sys.stderr)
            continue
    
    # Generate report
    detector.generate_report(all_regressions, args.output)
    detector.close()
    
    if all_regressions:
        print(f"❌ Found {len(all_regressions)} performance regressions")
        return 1
    else:
        print("✅ No performance regressions detected")
        return 0

if __name__ == "__main__":
    sys.exit(main())
