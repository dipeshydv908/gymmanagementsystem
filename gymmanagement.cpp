#include <algorithm>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

struct Date {
    int year;
    int month;
    int day;
};

struct Member {
    int id;
    string name;
    string phone;
    string email;
    int planId;
    Date startDate;
    Date endDate;
    bool active;
};

struct MembershipPlan {
    int id;
    string name;
    int durationDays;
    long long priceCents;
    bool active;
};

struct Payment {
    int id;
    int memberId;
    int planId;
    long long amountCents;
    Date paymentDate;
};

bool isLeapYear(int year) {
    return year % 400 == 0 || (year % 4 == 0 && year % 100 != 0);
}

bool isValidDate(const Date& date) {
    if (date.year < 1900 || date.year > 9999 ||
        date.month < 1 || date.month > 12 || date.day < 1)
        return false;

    const int daysInMonth[] = {
        31, isLeapYear(date.year) ? 29 : 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };
    return date.day <= daysInMonth[date.month - 1];
}

long long dateToSerial(const Date& date) {
    int year = date.year;
    const int month = date.month;
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yearOfEra = static_cast<unsigned>(year - era * 400);
    const unsigned dayOfYear =
        (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + date.day - 1;
    const unsigned dayOfEra =
        yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    return static_cast<long long>(era) * 146097 + dayOfEra;
}

Date serialToDate(long long serial) {
    const long long era = (serial >= 0 ? serial : serial - 146096) / 146097;
    const unsigned dayOfEra =
        static_cast<unsigned>(serial - era * 146097);
    const unsigned yearOfEra =
        (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524 -
         dayOfEra / 146096) / 365;
    int year = static_cast<int>(yearOfEra + era * 400);
    const unsigned dayOfYear =
        dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
    const unsigned monthPart = (5 * dayOfYear + 2) / 153;
    const unsigned day = dayOfYear - (153 * monthPart + 2) / 5 + 1;
    const int month = static_cast<int>(monthPart) + (monthPart < 10 ? 3 : -9);
    year += month <= 2;
    return {year, month, static_cast<int>(day)};
}

Date today() {
    const time_t now = time(nullptr);
    const tm* local = localtime(&now);
    if (local == nullptr)
        return {1970, 1, 1};
    return {local->tm_year + 1900, local->tm_mon + 1, local->tm_mday};
}

string formatDate(const Date& date) {
    ostringstream result;
    result << setfill('0') << setw(4) << date.year << '-'
           << setw(2) << date.month << '-' << setw(2) << date.day;
    return result.str();
}

bool parseDate(const string& text, Date& date) {
    if (text.size() != 10 || text[4] != '-' || text[7] != '-')
        return false;

    for (size_t i = 0; i < text.size(); ++i) {
        if (i != 4 && i != 7 && (text[i] < '0' || text[i] > '9'))
            return false;
    }

    date = {
        stoi(text.substr(0, 4)),
        stoi(text.substr(5, 2)),
        stoi(text.substr(8, 2))
    };
    return isValidDate(date);
}

string readLine(const string& prompt) {
    cout << prompt;
    string value;
    getline(cin, value);
    return value;
}

int readInteger(const string& prompt, int minimum, int maximum) {
    while (true) {
        const string input = readLine(prompt);
        istringstream parser(input);
        int value;
        char extra;
        if ((parser >> value) && !(parser >> extra) &&
            value >= minimum && value <= maximum)
            return value;
        cout << "Invalid input. Enter a whole number from " << minimum
             << " to " << maximum << ".\n";
    }
}

long long readMoney(const string& prompt) {
    while (true) {
        const string input = readLine(prompt);
        istringstream parser(input);
        double value;
        char extra;
        if ((parser >> value) && !(parser >> extra) &&
            isfinite(value) && value > 0 && value <= 1000000000.0)
            return static_cast<long long>(llround(value * 100.0));
        cout << "Enter a valid positive amount (maximum Rs. 1,000,000,000).\n";
    }
}

string readText(const string& prompt) {
    while (true) {
        string value = readLine(prompt);
        if (!value.empty() && value.find('|') == string::npos)
            return value;
        cout << "Value cannot be empty or contain the | character.\n";
    }
}

bool parseInteger(const string& text, int& value) {
    istringstream parser(text);
    char extra;
    return (parser >> value) && !(parser >> extra);
}

bool parseLongLong(const string& text, long long& value) {
    istringstream parser(text);
    char extra;
    return (parser >> value) && !(parser >> extra);
}

vector<string> split(const string& line, char delimiter) {
    vector<string> fields;
    string field;
    istringstream parser(line);
    while (getline(parser, field, delimiter))
        fields.push_back(field);
    return fields;
}

string moneyText(long long cents) {
    ostringstream result;
    result << "Rs. " << cents / 100 << '.' << setfill('0')
           << setw(2) << cents % 100;
    return result.str();
}

class GymManagementSystem {
private:
    vector<Member> members;
    vector<MembershipPlan> plans;
    vector<Payment> payments;

    const string membersFile = "gym_members.txt";
    const string plansFile = "gym_plans.txt";
    const string paymentsFile = "gym_payments.txt";

    Member* findMember(int id) {
        for (Member& member : members) {
            if (member.id == id)
                return &member;
        }
        return nullptr;
    }

    const MembershipPlan* findPlan(int id) const {
        for (const MembershipPlan& plan : plans) {
            if (plan.id == id)
                return &plan;
        }
        return nullptr;
    }

    int nextMemberId() const {
        int nextId = 1;
        for (const Member& member : members)
            nextId = max(nextId, member.id + 1);
        return nextId;
    }

    int nextPlanId() const {
        int nextId = 1;
        for (const MembershipPlan& plan : plans)
            nextId = max(nextId, plan.id + 1);
        return nextId;
    }

    int nextPaymentId() const {
        int nextId = 1;
        for (const Payment& payment : payments)
            nextId = max(nextId, payment.id + 1);
        return nextId;
    }

    bool saveAll() const {
        ofstream memberOutput(membersFile.c_str(), ios::trunc);
        if (!memberOutput) {
            cerr << "Error: could not save " << membersFile << ".\n";
            return false;
        }
        for (const Member& member : members) {
            memberOutput << member.id << '|' << member.name << '|'
                         << member.phone << '|' << member.email << '|'
                         << member.planId << '|' << formatDate(member.startDate)
                         << '|' << formatDate(member.endDate) << '|'
                         << member.active << '\n';
        }
        memberOutput.close();
        if (!memberOutput) {
            cerr << "Error: failed while writing " << membersFile << ".\n";
            return false;
        }

        ofstream planOutput(plansFile.c_str(), ios::trunc);
        if (!planOutput) {
            cerr << "Error: could not save " << plansFile << ".\n";
            return false;
        }
        for (const MembershipPlan& plan : plans) {
            planOutput << plan.id << '|' << plan.name << '|'
                       << plan.durationDays << '|' << plan.priceCents << '|'
                       << plan.active << '\n';
        }
        planOutput.close();
        if (!planOutput) {
            cerr << "Error: failed while writing " << plansFile << ".\n";
            return false;
        }

        ofstream paymentOutput(paymentsFile.c_str(), ios::trunc);
        if (!paymentOutput) {
            cerr << "Error: could not save " << paymentsFile << ".\n";
            return false;
        }
        for (const Payment& payment : payments) {
            paymentOutput << payment.id << '|' << payment.memberId << '|'
                          << payment.planId << '|' << payment.amountCents
                          << '|' << formatDate(payment.paymentDate) << '\n';
        }
        paymentOutput.close();
        if (!paymentOutput) {
            cerr << "Error: failed while writing " << paymentsFile << ".\n";
            return false;
        }
        return true;
    }

    void loadMembers() {
        ifstream input(membersFile.c_str());
        string line;
        int lineNumber = 0;
        while (getline(input, line)) {
            ++lineNumber;
            const vector<string> fields = split(line, '|');
            Member member{};
            if (fields.size() != 8 ||
                !parseInteger(fields[0], member.id) ||
                !parseInteger(fields[4], member.planId) ||
                !parseDate(fields[5], member.startDate) ||
                !parseDate(fields[6], member.endDate) ||
                (fields[7] != "0" && fields[7] != "1")) {
                cerr << "Warning: skipped invalid member record on line "
                     << lineNumber << ".\n";
                continue;
            }
            member.name = fields[1];
            member.phone = fields[2];
            member.email = fields[3];
            member.active = fields[7] == "1";
            members.push_back(member);
        }
    }

    void loadPlans() {
        ifstream input(plansFile.c_str());
        string line;
        int lineNumber = 0;
        while (getline(input, line)) {
            ++lineNumber;
            const vector<string> fields = split(line, '|');
            MembershipPlan plan{};
            if (fields.size() != 5 ||
                !parseInteger(fields[0], plan.id) ||
                !parseInteger(fields[2], plan.durationDays) ||
                !parseLongLong(fields[3], plan.priceCents) ||
                (fields[4] != "0" && fields[4] != "1") ||
                plan.durationDays < 1 || plan.priceCents < 0) {
                cerr << "Warning: skipped invalid plan record on line "
                     << lineNumber << ".\n";
                continue;
            }
            plan.name = fields[1];
            plan.active = fields[4] == "1";
            plans.push_back(plan);
        }
    }

    void loadPayments() {
        ifstream input(paymentsFile.c_str());
        string line;
        int lineNumber = 0;
        while (getline(input, line)) {
            ++lineNumber;
            const vector<string> fields = split(line, '|');
            Payment payment{};
            if (fields.size() != 5 ||
                !parseInteger(fields[0], payment.id) ||
                !parseInteger(fields[1], payment.memberId) ||
                !parseInteger(fields[2], payment.planId) ||
                !parseLongLong(fields[3], payment.amountCents) ||
                !parseDate(fields[4], payment.paymentDate) ||
                payment.amountCents < 0) {
                cerr << "Warning: skipped invalid payment record on line "
                     << lineNumber << ".\n";
                continue;
            }
            payments.push_back(payment);
        }
    }

    void showPlans(bool activeOnly = false) const {
        bool found = false;
        cout << "\n---------------- MEMBERSHIP PLANS ----------------\n";
        cout << left << setw(8) << "ID" << setw(24) << "Plan"
             << setw(16) << "Duration(days)" << setw(18) << "Price"
             << "Status\n";
        for (const MembershipPlan& plan : plans) {
            if (activeOnly && !plan.active)
                continue;
            found = true;
            cout << left << setw(8) << plan.id << setw(24) << plan.name
                 << setw(16) << plan.durationDays
                 << setw(18) << moneyText(plan.priceCents)
                 << (plan.active ? "Active" : "Inactive") << '\n';
        }
        if (!found)
            cout << "No plans found.\n";
    }

    int chooseActivePlan() const {
        showPlans(true);
        bool hasActivePlan = false;
        for (const MembershipPlan& plan : plans)
            hasActivePlan = hasActivePlan || plan.active;
        if (!hasActivePlan) {
            cout << "Create or reactivate a plan before continuing.\n";
            return -1;
        }
        const int id = readInteger("Enter plan ID: ", 1,
                                   numeric_limits<int>::max());
        const MembershipPlan* plan = findPlan(id);
        if (plan == nullptr || !plan->active) {
            cout << "That active plan was not found.\n";
            return -1;
        }
        return id;
    }

    void addPayment(Member& member, const MembershipPlan& plan,
                    const Date& startDate) {
        const Date endDate = serialToDate(
            dateToSerial(startDate) + plan.durationDays);
        const Date paymentDate = today();

        member.planId = plan.id;
        member.startDate = startDate;
        member.endDate = endDate;
        payments.push_back({
            nextPaymentId(), member.id, plan.id, plan.priceCents, paymentDate
        });

        cout << "Payment recorded: " << moneyText(plan.priceCents) << '\n'
             << "Membership valid from " << formatDate(startDate)
             << " through " << formatDate(endDate) << ".\n";
    }

    string membershipStatus(const Member& member) const {
        if (!member.active)
            return "Inactive account";
        const long long daysLeft = dateToSerial(member.endDate) -
                                   dateToSerial(today());
        if (daysLeft < 0)
            return "Expired";
        if (daysLeft <= 30)
            return "Expires in " + to_string(daysLeft) + " day(s)";
        return "Active";
    }

    void displayMember(const Member& member) const {
        const MembershipPlan* plan = findPlan(member.planId);
        cout << "Member ID      : " << member.id << '\n'
             << "Name           : " << member.name << '\n'
             << "Phone          : " << member.phone << '\n'
             << "Email          : " << member.email << '\n'
             << "Plan           : "
             << (plan == nullptr ? "Plan no longer available" : plan->name)
             << '\n'
             << "Valid from     : " << formatDate(member.startDate) << '\n'
             << "Valid through  : " << formatDate(member.endDate) << '\n'
             << "Status         : " << membershipStatus(member) << '\n';
    }

    void registerMember() {
        const int planId = chooseActivePlan();
        if (planId < 0)
            return;

        Member member{};
        member.id = nextMemberId();
        member.name = readText("Member name: ");
        member.phone = readText("Phone number: ");
        member.email = readText("Email address: ");
        member.active = true;

        const MembershipPlan* plan = findPlan(planId);
        if (plan == nullptr) {
            cout << "Selected plan is no longer available.\n";
            return;
        }
        addPayment(member, *plan, today());
        members.push_back(member);
        if (saveAll())
            cout << "Member registered successfully. Member ID: "
                 << member.id << '\n';
    }

    void listMembers() const {
        if (members.empty()) {
            cout << "No member records found.\n";
            return;
        }
        for (const Member& member : members) {
            cout << "\n----------------------------------------\n";
            displayMember(member);
        }
    }

    void searchMember() const {
        const int id = readInteger("Enter member ID: ", 1,
                                   numeric_limits<int>::max());
        for (const Member& member : members) {
            if (member.id == id) {
                displayMember(member);
                return;
            }
        }
        cout << "Member not found.\n";
    }

    void updateMember() {
        const int id = readInteger("Enter member ID to update: ", 1,
                                   numeric_limits<int>::max());
        Member* member = findMember(id);
        if (member == nullptr) {
            cout << "Member not found.\n";
            return;
        }

        cout << "Enter the updated contact details.\n";
        member->name = readText("Member name: ");
        member->phone = readText("Phone number: ");
        member->email = readText("Email address: ");
        if (saveAll())
            cout << "Member details updated successfully.\n";
    }

    void changeMemberStatus() {
        const int id = readInteger("Enter member ID: ", 1,
                                   numeric_limits<int>::max());
        Member* member = findMember(id);
        if (member == nullptr) {
            cout << "Member not found.\n";
            return;
        }
        member->active = !member->active;
        if (saveAll())
            cout << "Member account " << (member->active ? "activated" : "deactivated")
                 << ".\n";
    }

    void membersMenu() {
        int choice;
        do {
            cout << "\n===== MEMBER MANAGEMENT =====\n"
                 << "1. Register member\n"
                 << "2. List members\n"
                 << "3. Search member\n"
                 << "4. Update member details\n"
                 << "5. Activate/deactivate member\n"
                 << "0. Back\n";
            choice = readInteger("Enter choice: ", 0, 5);
            switch (choice) {
                case 1: registerMember(); break;
                case 2: listMembers(); break;
                case 3: searchMember(); break;
                case 4: updateMember(); break;
                case 5: changeMemberStatus(); break;
            }
        } while (choice != 0);
    }

    void addPlan() {
        MembershipPlan plan{};
        plan.id = nextPlanId();
        plan.name = readText("Plan name: ");
        plan.durationDays = readInteger("Duration in days: ", 1, 3650);
        plan.priceCents = readMoney("Price (Rs.): ");
        plan.active = true;
        plans.push_back(plan);
        if (saveAll())
            cout << "Plan added successfully. Plan ID: " << plan.id << '\n';
    }

    void updatePlan() {
        showPlans();
        const int id = readInteger("Enter plan ID to update: ", 1,
                                   numeric_limits<int>::max());
        MembershipPlan* selected = nullptr;
        for (MembershipPlan& plan : plans) {
            if (plan.id == id) {
                selected = &plan;
                break;
            }
        }
        if (selected == nullptr) {
            cout << "Plan not found.\n";
            return;
        }
        selected->name = readText("New plan name: ");
        selected->durationDays = readInteger("New duration in days: ", 1, 3650);
        selected->priceCents = readMoney("New price (Rs.): ");
        if (saveAll())
            cout << "Plan updated. Existing payments remain unchanged.\n";
    }

    void changePlanStatus() {
        showPlans();
        const int id = readInteger("Enter plan ID: ", 1,
                                   numeric_limits<int>::max());
        for (MembershipPlan& plan : plans) {
            if (plan.id == id) {
                plan.active = !plan.active;
                if (saveAll())
                    cout << "Plan " << (plan.active ? "activated" : "deactivated")
                         << ".\n";
                return;
            }
        }
        cout << "Plan not found.\n";
    }

    void plansMenu() {
        int choice;
        do {
            cout << "\n===== MEMBERSHIP PLAN MANAGEMENT =====\n"
                 << "1. Add plan\n"
                 << "2. List plans\n"
                 << "3. Update plan\n"
                 << "4. Activate/deactivate plan\n"
                 << "0. Back\n";
            choice = readInteger("Enter choice: ", 0, 4);
            switch (choice) {
                case 1: addPlan(); break;
                case 2: showPlans(); break;
                case 3: updatePlan(); break;
                case 4: changePlanStatus(); break;
            }
        } while (choice != 0);
    }

    void listValidity() const {
        if (members.empty()) {
            cout << "No member records found.\n";
            return;
        }
        cout << "\n===== MEMBERSHIP VALIDITY =====\n";
        for (const Member& member : members) {
            cout << "\n----------------------------------------\n";
            displayMember(member);
        }
    }

    void renewMembership() {
        const int id = readInteger("Enter member ID to renew: ", 1,
                                   numeric_limits<int>::max());
        Member* member = findMember(id);
        if (member == nullptr) {
            cout << "Member not found.\n";
            return;
        }
        if (!member->active) {
            cout << "This member account is inactive. Activate it first.\n";
            return;
        }

        const int planId = chooseActivePlan();
        if (planId < 0)
            return;
        const MembershipPlan* plan = findPlan(planId);
        if (plan == nullptr) {
            cout << "Selected plan is no longer available.\n";
            return;
        }

        const Date currentDate = today();
        const Date startDate = dateToSerial(member->endDate) >=
                               dateToSerial(currentDate)
                                   ? member->endDate
                                   : currentDate;
        addPayment(*member, *plan, startDate);
        if (saveAll())
            cout << "Membership renewed successfully.\n";
    }

    void validityMenu() {
        int choice;
        do {
            cout << "\n===== VALIDITY & RENEWALS =====\n"
                 << "1. View all membership validity\n"
                 << "2. Show expired/expiring memberships\n"
                 << "3. Renew membership\n"
                 << "0. Back\n";
            choice = readInteger("Enter choice: ", 0, 3);
            switch (choice) {
                case 1: listValidity(); break;
                case 2: {
                    bool found = false;
                    for (const Member& member : members) {
                        if (member.active &&
                            membershipStatus(member) != "Active") {
                            displayMember(member);
                            cout << "----------------------------------------\n";
                            found = true;
                        }
                    }
                    if (!found)
                        cout << "No expired or soon-to-expire memberships.\n";
                    break;
                }
                case 3: renewMembership(); break;
            }
        } while (choice != 0);
    }

    void paymentHistory() const {
        if (payments.empty()) {
            cout << "No payment records found.\n";
            return;
        }
        cout << "\n---------------- PAYMENT HISTORY ----------------\n";
        cout << left << setw(10) << "Receipt" << setw(12) << "Member ID"
             << setw(10) << "Plan ID" << setw(16) << "Amount"
             << "Payment date\n";
        for (const Payment& payment : payments) {
            cout << left << setw(10) << payment.id << setw(12)
                 << payment.memberId << setw(10) << payment.planId
                 << setw(16) << moneyText(payment.amountCents)
                 << formatDate(payment.paymentDate) << '\n';
        }
    }

    bool readDateInput(const string& prompt, Date& date) const {
        while (true) {
            const string text = readLine(prompt);
            if (parseDate(text, date))
                return true;
            cout << "Enter a valid date in YYYY-MM-DD format.\n";
        }
    }

    void revenueReport() const {
        Date fromDate;
        Date toDate;
        readDateInput("Start date (YYYY-MM-DD): ", fromDate);
        readDateInput("End date (YYYY-MM-DD): ", toDate);
        if (dateToSerial(fromDate) > dateToSerial(toDate)) {
            cout << "Start date must not be later than end date.\n";
            return;
        }

        long long totalCents = 0;
        int count = 0;
        cout << "\n===== REVENUE REPORT =====\n"
             << "Period: " << formatDate(fromDate) << " to "
             << formatDate(toDate) << '\n';
        for (const Payment& payment : payments) {
            const long long paymentDay = dateToSerial(payment.paymentDate);
            if (paymentDay >= dateToSerial(fromDate) &&
                paymentDay <= dateToSerial(toDate)) {
                totalCents += payment.amountCents;
                ++count;
            }
        }
        cout << "Payments received : " << count << '\n'
             << "Total revenue     : " << moneyText(totalCents) << '\n';
    }

    void summaryReport() const {
        int activeMembers = 0;
        int expiredMembers = 0;
        int inactiveMembers = 0;
        int expiringSoon = 0;
        long long totalRevenue = 0;
        for (const Member& member : members) {
            if (!member.active) {
                ++inactiveMembers;
                continue;
            }
            const long long remainingDays = dateToSerial(member.endDate) -
                                           dateToSerial(today());
            if (remainingDays < 0)
                ++expiredMembers;
            else {
                ++activeMembers;
                if (remainingDays <= 30)
                    ++expiringSoon;
            }
        }
        for (const Payment& payment : payments)
            totalRevenue += payment.amountCents;

        cout << "\n===== GYM SUMMARY REPORT =====\n"
             << "Total members       : " << members.size() << '\n'
             << "Current memberships : " << activeMembers << '\n'
             << "Expired memberships: " << expiredMembers << '\n'
             << "Expiring within 30 days: " << expiringSoon << '\n'
             << "Inactive accounts   : " << inactiveMembers << '\n'
             << "Total plans         : " << plans.size() << '\n'
             << "Total payments      : " << payments.size() << '\n'
             << "All-time revenue    : " << moneyText(totalRevenue) << '\n';
    }

    void paymentsMenu() {
        int choice;
        do {
            cout << "\n===== PAYMENTS & REPORTS =====\n"
                 << "1. View payment history\n"
                 << "2. Revenue report by date range\n"
                 << "3. Gym summary report\n"
                 << "0. Back\n";
            choice = readInteger("Enter choice: ", 0, 3);
            switch (choice) {
                case 1: paymentHistory(); break;
                case 2: revenueReport(); break;
                case 3: summaryReport(); break;
            }
        } while (choice != 0);
    }

public:
    GymManagementSystem() {
        loadPlans();
        loadMembers();
        loadPayments();
    }

    void run() {
        int choice;
        do {
            cout << "\n========== GYM MEMBERSHIP SYSTEM ==========\n"
                 << "1. Register and manage members\n"
                 << "2. Manage membership plans\n"
                 << "3. Track validity and renewals\n"
                 << "4. Manage payments and generate reports\n"
                 << "0. Save and exit\n";
            choice = readInteger("Enter choice: ", 0, 4);
            switch (choice) {
                case 1: membersMenu(); break;
                case 2: plansMenu(); break;
                case 3: validityMenu(); break;
                case 4: paymentsMenu(); break;
                case 0:
                    if (saveAll())
                        cout << "Data saved. Goodbye!\n";
                    break;
            }
        } while (choice != 0);
    }
};

int main() {
    GymManagementSystem system;
    system.run();
    return 0;
}