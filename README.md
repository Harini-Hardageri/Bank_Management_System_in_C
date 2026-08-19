# Bank_Management_System_in_C

A console-based banking system that simulates core banking operations using structures, file handling, authentication, transaction management, loan processing, activity logging, and basic fraud detection mechanism.

# Key Features
<ul>
<li><b>User Authentication & Registration</b></li>
<ul>
<li>User registration with username and password validation.</li>
<li>Password masking during login.</li>
<li>Role-based access for Users and Administrators.</li>
<li>Account suspension and password reset functionality.</li>
<li>Failed login attempt detection and fraud alerts.</li>
</ul>

<li><b>Bank Account Management</b></li>
<ul>
<li>Creation of Savings Accounts by administrators.</li>
<li>Unique account number generation.</li>
<li>Customer profile and account information management.</li>
<li>Edit, search, view, and delete account functionality.</li>
<li>Balance and account details management.</li>
</ul>
  
<li><b>Banking Transactions</b></li>
<ul>
<li>Deposit money.</li>
<li>Withdraw money.</li>
<li>Transfer funds between accounts.</li>
<li>View transaction history.</li>
<li>Generate and save account statements.</li>
<li>Automatic transaction timestamps and transaction logging.</li>
</ul>

<li><b>Loan Management</b></li>
<ul>
<li>Customers can apply for loans.</li>
<li>Administrators can review pending loan applications.</li>
<li>Loan requests can be approved or rejected.</li>
<li>Users can view their loan application status.</li>
</ul>

<li><b>Fraud Detection & Security</b></li>
<ul>
<li>Detection of multiple failed login attempts.</li>
<li>Rapid withdrawal detection.</li>
<li>Daily withdrawal limit monitoring.</li>
<li>Large withdrawal and transfer alerts.</li>
<li>Fraud events are recorded in dedicated fraud logs.</li>
<li>Session timeout after inactivity.</li>
</ul>

<li><b>Logging & File-Based Storage</b></li>
<ul>
<li>User and administrator activities are recorded in system logs.</li>
<li>Binary files are used to store users, accounts, transactions, and loans.</li>
<li>Separate logs are maintained for fraud-related activities.</li>
<li>Bank statements can be generated as text files.</li>
</ul>
</ul>
