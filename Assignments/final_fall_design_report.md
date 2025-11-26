# OJ – Senior Design Final Fall Report  
University of Cincinnati  
Advisor: **Hrishikesh Bhide**

Team Members:  
- **Omkar Sontake (CS)** — sontakor@mail.uc.edu  
- **Jash Dedhia (CS & Finance)** — dedhiaja@mail.uc.edu  

GitHub Repository:  
https://github.github.com/OmkarSontake/OJ-Senior-design-

---

# 📘 Table of Contents

### Project Overview  
- [Team Names & Project Abstract](#team-names--project-abstract)  
- [Project Description (Assignment #2)](#project-description-assignment-2)

### Design & Requirements  
- [User Stories (Assignment #4)](#user-stories-assignment-4)  
- [Design Diagrams (D0, D1, D2)](#design-diagrams-level-0-level-1-level-2)  
- [Diagram Descriptions](#diagram-descriptions)

### Project Management  
- [Task List (Assignment #5–6)](#task-list-assignment-5-6)  
- [Timeline](#timeline)  
- [Effort Matrix](#effort-matrix)

### Ethics & Presentation  
- [ABET Concerns Essay (Assignment #7)](#abet-concerns-essay-assignment-7)  
- [PPT Slideshow Summary (Assignment #8)](#ppt-slideshow-summary-assignment-8)

### Team Deliverables  
- [Self-Assessment Essays (Assignment #3)](#self-assessment-essays-assignment-3)  
- [Professional Biographies (Assignment #1)](#professional-biographies-assignment-1)

### Administrative  
- [Budget](#budget)  
- [Appendix](#appendix)

---

# Team Names & Project Abstract

**Team OJ**  
Advisor: **Hrishikesh Bhide**

**Abstract**  
We are developing a platform that backtests trading strategies using historical market data. Users can upload datasets, define trading rules, simulate trades, and review performance metrics. The system helps students and new investors safely explore algorithmic trading ideas without financial risk.

---

# Project Description (Assignment #2)

We are building a platform for backtesting trading strategies. Many new investors want to test trading ideas but do not have access to expensive tools or professional platforms. With our project we aim to gives users the ability to upload historical price data, define simple rule-based trading strategies, run a backtest simulation, and view performance results.

We have designed the platform to be approachable for business and CS students as well as beginners. It includes modules that load data, evaluate strategy rules, simulate buy/sell operations over time, track portfolio value, and generate useful metrics and summaries. The goal is to make algorithmic trading concepts easier to understand while giving users a safe way to experiment with financial data.

---

# User Stories (Assignment #4)

**1.** As a user, I want to upload historical price data so I can test strategies.  
**2.** As a user, I want to define simple buy/sell conditions.  
**3.** As a user, I want to run a backtest and view the performance.  
**4.** As a user, I want clear metrics like returns and win/loss ratio.  
**5.** As a user, I want charts showing price movements and trades.  
**6.** As a user, I want the system to be easy enough for non-technical users.

---

# Design Diagrams (Level 0, Level 1, Level 2)

### **Level 0 Diagram – Context Level**
- User uploads data + defines rules  
- System processes backtest  
- System outputs metrics + charts  

### **Level 1 Diagram – Subsystems**
1. Data Loader  
2. Strategy Engine  
3. Simulation Engine  
4. Metrics Calculator  
5. Report Generator  

### **Level 2 Diagram – Detailed Flow**
- Data Loader → parses rows  
- Strategy Engine → checks rules  
- Simulation Engine → executes trades  
- Metrics Calculator → generates stats  
- Report Generator → outputs visuals  

---

# Diagram Descriptions

### **Data Loader**
Reads & validates CSV data (price, volume, timestamps).  

### **Strategy Engine**
Evaluates rule conditions like:  
- Price > MA  
- Price crossovers  
- Threshold indicators  

### **Simulation Engine**
Executes buy/sell operations, updates portfolio, tracks positions.  

### **Metrics Module**
Computes total return, average return, drawdowns, win/loss ratio, number of trades.

### **Report Generator**
Produces charts, tables, and summaries for user review.

---

# Task List (Assignment #5–6)

Tasks completed during Fall semester:
- Research financial backtesting platforms  
- Write user stories & requirements  
- Build early architecture diagrams  
- Create data ingestion system  
- Develop strategy rule evaluator  
- Build backtesting simulation logic  
- Generate portfolio metrics  
- Create presentation slides  
- Develop full Fall report  

---

# Timeline

### **September**
- Requirements gathering  (Comleted)
- Architecture planning   (Comleted)

### **October**
- User stories  (Comleted)
- Design diagrams D0–D2  (Comleted)

### **November**
- Data loader  (Comleted)
- Strategy engine  (Work in progress)

### **December**
- Backtest simulator  
- Metrics calculations  

### **Jan–March**
- Improvements  
- Code cleanup  
- Additional testing  

### **April**
- Final report  
- Final presentation  

---

# Effort Matrix

Each team member completed at least **45 hours**.

| Team Member | Research | Coding | Documentation | Testing | Total |
|------------|----------|--------|---------------|---------|--------|
| Jash       | 12 hrs   | 14 hrs | 9 hrs         | 10 hrs  | 45 hrs |
| Omkar      | 10 hrs   | 15 hrs | 10 hrs        | 10 hrs  | 45 hrs |

---

# ABET Concerns Essay (Assignment #7)

### Ethical Considerations:
- Handle financial data responsibly  
- Avoid misleading users with improper simulations  
- Maintain transparency in how strategies are evaluated  
- Provide correct information about system limitations  
- Avoid creating false expectations for real trading  

Our project a financial back-testing and portfolio optimization framework operates under
several key constraints that guide both its design and implementation.
From a technical perspective we have used open source libraries like Pandas, NumPy, and
Matplotlib in Python. On a professional level, this project helps us strengthen our technical
skills in quantitative finance, algorithmic trading, and software design. At the same time, it
challenges us develop coding skills, use documentation, and clear collaboration to reflect
professional competence and teamwork.
From an ethical perspective, we’re committed to developing features for educational and
analytical purposes and not to mislead financial predictions or provide any form of investment
advice. We prioritize transparency by clearly stating our assumptions and methods to uphold
academic integrity.
We will follow all data usage rights and keep our datasets secure through local storage and
careful file handling to prevent unauthorized access or data tampering.
Lastly our goal is so that the project supports accessible quantitative finance education without
causing environmental harm, as it operates entirely in a digital environment and promotes
open learning.
Together, these constraints help us build a project that is responsible, efficient, and ethically
sound, striking a thoughtful balance between innovation and accountability


---

# PPT Slideshow Summary (Assignment #8)

Main presentation points:
- Project introduction  
- Market problem / motivation  
- Requirements and user stories  
- Architecture diagrams (D0–D2)  
- Full system demo  
- Backtesting examples  
- Performance metrics  
- ABET concerns  
- Future improvements  
- Conclusion  
Here is a link to the slides

https://github.com/OmkarSontake/OJ-Senior-design-/blob/main/Assignments/Senior_Design_Project_Slides%20(1)%20(1).pptx

---

# Self-Assessment Essays (Assignment #3)

### **Omkar Sontake – Self Assessment**
*Omkar Sontake
9/15/2025
Through our senior design project, we hope to create an application that can be used to
backtest strategies. The goal is to design a tool that lets users test trading and investment
ideas with historical data so they can understand how those strategies might perform
before applying them in real markets. As a Computer Science major with a Finance minor, I
see this project connects my two passions. It allows me to apply my passion for
technology to solve problems in another industry. On the side note having a huge passion
for finance, I am also looking forward to using it myself.
The courses I have taken at the University of Cincinnati have prepared me to take on this
project are data structures which trained me to think about efficiency and performance.
CS 4033 provided me with exposure to methods that could make back testing strategies
more powerful. CS 5127 (Requirements Engineering) helped me understand how to gather
and manage user requirements to make sure the platform meets practical needs. Finally,
CS 5168 (Parallel Computing) and EECE 5132 (Software Testing & Quality Assurance) gave
me the skills to improve system speed and ensure reliability through structured testing.
These courses gave me both the technical and problem-solving mindset needed for this
project.
My internship experience as an IT Audit Intern at Fidelity also prepared me with practical
skills. At Fidelity, I developed Python scripts to automate compliance and testing tasks,
which strengthened my coding and problem-solving abilities. I also learned about risk
management and how to explain technical work clearly to people from non-technical
backgrounds. These skills will be directly useful as we build a system that must be easy to
understand. Working with Jash, who also interned at Fidelity, adds even more strength to
our project because we both bring experience in finance and technology, as well as
familiarity with teamwork in a professional setting.
I am passionate about this project because it combines my passion for computer science
and finance, I am excited about the chance to design a system that could help traders.
Building a tool that goes beyond academics and could be used by traders and people like
me is very intresting to me. I am also looking forward to working with Jash, who shares
similar interests/passion. Working with him I hope to learn to grow not just as a
professional but individual aswell.
We plan on setting deadlines and goals to ensure progress is completed in a timely
manner. First, we plan to identify requirements and select the data we will use for
backtesting. Then, we plan on building the platform brick by brick. I expect our results to
include a working platform with simulations that provide quantifiable results for our users.
To evaluate myself, I will check whether my work is completed by deadlines and integrates
smoothly with Jash’s work. Our end goal with this project is to develop a platform that is
reliable, and able to provide financial insights.*
.

---

# Professional Biographies (Assignment #1)

### **Omkar Sontake**
Computer Science student interested in backend systems and data engineering. Worked on strategy engine, data processing, diagrams, timeline, and testing.

### **Jash Dedhia**
Computer Science and Finance student interested in algorithmic trading. Worked on user stories, testing strategies, diagrams, and financial logic.

---

# Budget

- No expenses to date  
- No purchased tools  
- No donated resources  
- All tools used were free (GitHub, VSCode, Python libraries)

---

# Appendix

Included materials:
- User stories  
- Full diagrams  
- Self-assessments  
- Team contract  
- ABET concerns essay  
- Task list  
- Milestones timeline  
- Effort matrix  
- Meeting notes  
- GitHub repository  
- All required documentation  



---
# **END OF README.md**
