Specify data sources and schemas for historical market data ingestion (CSV/API), including required fields (timestamp, open, high, low, close, volume). Responsible: Jash Dedhia
Develop the Data Import module to load OHLCV data from local CSVs with robust file validation and error handling. Responsible: Jash Dedhia
Implement data cleaning routines to handle missing values, duplicate timestamps, and timezone normalization. Responsible: Jash Dedhia
Document data assumptions and preprocessing steps (imputation rules, outlier policy, trading calendar). Responsible: Jash Dedhia
Design the strategy rule interface (function signatures / configuration spec) for simple buy/sell rules. Responsible: Omkar Sontake
Develop the signal generation component to transform rule definitions into entry/exit signals. Responsible: Omkar Sontake
Implement an execution simulator that converts signals to trades, applying fees and slippage consistently. Responsible: Omkar Sontake
Validate execution logic with unit tests covering edge cases (gaps, partial fills, holidays). Responsible: Omkar Sontake
Compute core performance metrics CAGR, Sharpe ratio, maximum drawdown, win rate, and trade statistics. Responsible: Jash Dedhia
Generate equity curve and drawdown charts for single runs, ensuring axes/labels and exportable images. Responsible: Jash Dedhia
Produce standardized CSV outputs (trades, daily equity, summary metrics) for reproducibility. Responsible: Jash Dedhia
Draft results documentation describing metric definitions and chart interpretation. Responsible: Jash Dedhia
Integrate module interfaces (Data Loader ↔ Strategy Logic ↔ Results) inside the Backtest Engine orchestrator. Responsible: Omkar Sontake
Add configuration management (YAML/JSON) to parameterize data paths, strategy params, and cost assumptions. Responsible: Omkar Sontake
Implement run logging (start/end time, commit hash, parameters) for experiment traceability. Responsible: Omkar Sontake
Create a minimal CLI entry point to run backtests from the command line with arguments. Responsible: Omkar Sontake
Design and execute system tests that run an end-to-end backtest on sample data to verify outputs match expectations. Responsible: Omkar Sontake
Establish a baseline strategy (e.g., SMA crossover) and record reference results for regression checks. Responsible: Jash Dedhia
Refine performance and memory usage (vectorization, batching I/O) for medium datasets. Responsible: Jash Dedhia
Prepare final project documentation (README updates, module overviews, how-to-run) and export a PDF for submission. Responsible: Omkar Sontake
