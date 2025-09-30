# Backtesting Project Task Assignments

1. Specify data sources and schemas for historical market data ingestion (CSV/API), including required fields (timestamp, open, high, low, close, volume). Responsible: Jash Dedhia  
2. Develop the Data Import module to load OHLCV data from local CSVs with robust file validation and error handling. Responsible: Jash Dedhia  
3. Implement data cleaning routines to handle missing values, duplicate timestamps, and timezone normalization. Responsible: Jash Dedhia  
4. Document data assumptions and preprocessing steps (imputation rules, outlier policy, trading calendar). Responsible: Jash Dedhia  
5. Design the strategy rule interface (function signatures / configuration spec) for simple buy/sell rules. Responsible: Omkar Sontake  
6. Develop the signal generation component to transform rule definitions into entry/exit signals. Responsible: Omkar Sontake  
7. Implement an execution simulator that converts signals to trades, applying fees and slippage consistently. Responsible: Omkar Sontake  
8. Validate execution logic with unit tests covering edge cases (gaps, partial fills, holidays). Responsible: Omkar Sontake  
9. Compute core performance metrics CAGR, Sharpe ratio, maximum drawdown, win rate, and trade statistics. Responsible: Jash Dedhia  
10. Generate equity curve and drawdown charts for single runs, ensuring axes/labels and exportable images. Responsible: Jash Dedhia  
11. Produce standardized CSV outputs (trades, daily equity, summary metrics) for reproducibility. Responsible: Jash Dedhia  
12. Draft results documentation describing metric definitions and chart interpretation. Responsible: Jash Dedhia  
13. Integrate module interfaces (Data Loader ↔ Strategy Logic ↔ Results) inside the Backtest Engine orchestrator. Responsible: Omkar Sontake  
14. Add configuration management (YAML/JSON) to parameterize data paths, strategy params, and cost assumptions. Responsible: Omkar Sontake  
15. Implement run logging (start/end time, commit hash, parameters) for experiment traceability. Responsible: Omkar Sontake  
16. Create a minimal CLI entry point to run backtests from the command line with arguments. Responsible: Omkar Sontake  
17. Design and execute system tests that run an end-to-end backtest on sample data to verify outputs match expectations. Responsible: Omkar Sontake  
18. Establish a baseline strategy (e.g., SMA crossover) and record reference results for regression checks. Responsible: Jash Dedhia  
19. Refine performance and memory usage (vectorization, batching I/O) for medium datasets. Responsible: Jash Dedhia  
20. Prepare final project documentation (README updates, module overviews, how-to-run) and export a PDF for submission. Responsible: Omkar Sontake  
