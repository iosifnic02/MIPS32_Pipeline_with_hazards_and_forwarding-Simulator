#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <iomanip>
#include <stdexcept>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <cctype>
using namespace std;

//---------------------------
// Instruction types and control signals
//---------------------------
enum class InstrType {
    R_TYPE, I_TYPE, MEM_TYPE, BRANCH_TYPE, SHIFT_TYPE, NOP_TYPE
};

// Δημιουργία δομής Monitor με 27 πεδία (οι πρώτες 24 όπως πριν και 3 για το WB)
struct Monitor {
    // IF: monitors 1-3
    string monitor1;
    string monitor2;
    string monitor3;
    // ID: monitors 4-15 (12 πεδία)
    string monitor4;
    string monitor5;
    string monitor6;
    string monitor7;
    string monitor8;
    string monitor9;
    string monitor10;
    string monitor11;
    string monitor12;
    string monitor13;
    string monitor14;
    string monitor15;
    // EX: monitors 16-19 (4 πεδία)
    string monitor16;
    string monitor17;
    string monitor18;
    string monitor19;
    // MEM: monitors 20-24 (5 πεδία)
    string monitor20;
    string monitor21;
    string monitor22;
    string monitor23;
    string monitor24;
    // WB: monitors 25-28 (τρία πεδία τώρα)
    string monitor25;
    string monitor26;
    string monitor27;
    string monitor28;

    // Κατασκευαστής: αρχικοποίηση όλων με "-"
    Monitor() : monitor1("-"), monitor2("-"), monitor3("-"),
                monitor4("-"), monitor5("-"), monitor6("-"), monitor7("-"),
                monitor8("-"), monitor9("-"), monitor10("-"), monitor11("-"),
                monitor12("-"), monitor13("-"), monitor14("-"), monitor15("-"),
                monitor16("-"), monitor17("-"), monitor18("-"), monitor19("-"),
                monitor20("-"), monitor21("-"), monitor22("-"), monitor23("-"),
                monitor24("-"), monitor25("-"), monitor26("-"), monitor27("-") { }
};

//---------------------------
// Structures for instructions and labels
//---------------------------
struct Instruction {
    string mnemonic;      // π.χ. "add", "lw", "ori", κ.λπ.
    InstrType type;       // τύπος εντολής
    int rs = 0;
    int rt = 0;
    int rd = 0;
    int immediate = 0;
    string labelTarget;
    int PC = 0;
    string fullLine;      // ολόκληρη η γραμμή (για output)
    int sltresult;
};

struct Label {
    string name;
    int address;
};

//---------------------------
// Memory class – διαχείριση μνήμης
//---------------------------
class Memory {
public:
    map<int, int> mem;
    int read(int address) {
        if (mem.find(address) != mem.end())
            return mem[address];
        return 0;
    }
    void write(int address, int value) {
        mem[address] = value;
    }
    string getMemoryState() {
        if (mem.empty()) return "";
        vector<int> addresses;
        for (auto &p : mem)
            addresses.push_back(p.first);
        sort(addresses.begin(), addresses.end());
        ostringstream oss;
        for (size_t i = 0; i < addresses.size(); i++) {
            oss << toHex(mem[addresses[i]]);
            if (i < addresses.size() - 1)
                oss << "\t";
        }
        return oss.str();
    }
private:
    string toHex(int num) {
        unsigned int u = static_cast<unsigned int>(num);
        stringstream ss;
        ss << hex << nouppercase << u;
        return ss.str();
    }
};

//---------------------------
// CPU class – διαχείριση register file
//---------------------------
class CPU {
public:
    vector<int> registers;
    vector<int> registersfinal;
    CPU() {
        registers.resize(33, 0);
        registersfinal.resize(33, 0);
        registers[28] = 0x10008000;
        registers[29] = 0x7ffffffc;
        registersfinal[28] = 0x10008000;
        registersfinal[29] = 0x7ffffffc;
    }
};

//---------------------------
// Utility functions για parsing και formatting
//---------------------------
string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

vector<string> split(const string &s, const string &delims = " ,\t") {
    vector<string> tokens;
    size_t start = s.find_first_not_of(delims), end = 0;
    while (start != string::npos) {
        end = s.find_first_of(delims, start);
        tokens.push_back(s.substr(start, end - start));
        start = s.find_first_not_of(delims, end);
    }
    return tokens;
}

string removeComma(const string &s) {
    string res = s;
    res.erase(remove(res.begin(), res.end(), ','), res.end());
    return res;
}

int getRegisterNumber(const string &regStr) {
    string r = removeComma(regStr);
    if (!r.empty() && r[0] == '$')
        r = r.substr(1);
    if (r == "r0") return 0;
    if (r == "at") return 1;
    if (r == "v0") return 2;
    if (r == "v1") return 3;
    if (r == "a0") return 4;
    if (r == "a1") return 5;
    if (r == "a2") return 6;
    if (r == "a3") return 7;
    if (r == "t0") return 8;
    if (r == "t1") return 9;
    if (r == "t2") return 10;
    if (r == "t3") return 11;
    if (r == "t4") return 12;
    if (r == "t5") return 13;
    if (r == "t6") return 14;
    if (r == "t7") return 15;
    if (r == "s0") return 16;
    if (r == "s1") return 17;
    if (r == "s2") return 18;
    if (r == "s3") return 19;
    if (r == "s4") return 20;
    if (r == "s5") return 21;
    if (r == "s6") return 22;
    if (r == "s7") return 23;
    if (r == "t8") return 24;
    if (r == "t9") return 25;
    if (r == "k0") return 26;
    if (r == "k1") return 27;
    if (r == "gp") return 28;
    if (r == "sp") return 29;
    if (r == "fp") return 30;
    if (r == "ra") return 31;
    if (r == "zero") return 32;
    return -1;
}

string getRegisterName(int reg) {
    static vector<string> names = {"r0", "at", "v0", "v1", "a0", "a1", "a2", "a3",
                                   "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
                                   "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
                                   "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra", "zero"};
    if (reg >= 0 && reg < (int)names.size())
        return "$" + names[reg];
    return "$?";
}

int parseNumber(const string &str) {
    string s = trim(str);
    if (s.size() >= 2 && s[0]=='0' && (s[1]=='x' || s[1]=='X')) {
        try {
            return stoi(s.substr(2), nullptr, 16);
        } catch (const std::exception &e) {
            throw;
        }
    } else {
        try {
            return stoi(s, nullptr, 10);
        } catch (const std::exception &e) {
            throw;
        }
    }
}

int convertImmediate(const string &str) {
    return parseNumber(str);
}

string toHex(int num) {
    unsigned int u = static_cast<unsigned int>(num);
    stringstream ss;
    ss << hex << nouppercase << u;
    return ss.str();
}

// Βοηθητική συνάρτηση για δημιουργία NOP (bubble)
Instruction createBubble() {
    Instruction nop;
    nop.mnemonic = "NOP";
    nop.type = InstrType::NOP_TYPE;
    nop.PC = 0;
    nop.fullLine = "NOP";
    return nop;
}

//---------------------------
// Simulator class – 5-stage pipeline με hazard detection & forwarding
//---------------------------
class Simulator {
private:
    vector<Instruction> instructions;
    vector<Label> labels;
    CPU cpu;
    Memory memory;
    int pc;          // εσωτερικός program counter
    int cycleCount;  // μετρητής κύκλων
    bool branchTakenFlag = false;
    int branchTargetAddress = 0;
    int sllflagtelos = -1;
    string EXE_monitor16,EXE_monitor17;
    int ALU_RESULT;
    string monitor_7itype="-";
    string monitor_8itype="-";

    // Flags για ένδειξη hazard και forwarding – για χρήση στα monitors
    bool hazardFlag = false;
    bool forwardingFlag = false;

    // Δημιουργία 5 αντικειμένων Monitor για τα pipeline stages
    Monitor ifMonitor;
    Monitor idMonitor;
    Monitor exMonitor;
    Monitor memMonitor;
    Monitor wbMonitor;

    // Pipeline register structure (ένα ανά στάδιο)
    struct PipelineRegister {
        Instruction instr;
        // Για απλούστευση δεν περιλαμβάνουμε πλέον πλήρη control signals
        int aluResult;
        bool valid;
        string monitoritype11;

        bool HazardTriggered = false;   // Θα γίνεται true αν στην ID_stage εντοπιστεί hazard
        bool ForwardingUsed = false;      // Θα γίνεται true αν στην EX_stage χρησιμοποιηθεί forwarding


        PipelineRegister() : instr(createBubble()), aluResult(0), valid(false) { }
    };

    PipelineRegister IF_reg, ID_reg, EX_reg, MEM_reg, WB_reg;

    //---------------------------
    // Parsing functions
    //---------------------------
    void parseLine(const string &line, int currentPC) {
        string trimmed = trim(line);
        size_t commentPos = trimmed.find('#');
        if (commentPos != string::npos) {
            trimmed = trim(trimmed.substr(0, commentPos));
        }
        if (trimmed.empty() || trimmed[0] == '.')
            return;
        size_t colonPos = trimmed.find(':');
        if (colonPos != string::npos) {
            string labName = trim(trimmed.substr(0, colonPos));
            labels.push_back(Label{labName, currentPC});
            if (colonPos + 1 < trimmed.size()) {
                string rest = trim(trimmed.substr(colonPos + 1));
                if (!rest.empty())
                    parseInstruction(rest, currentPC);
            }
        } else {
            parseInstruction(trimmed, currentPC);
        }
    }

    void parseInstruction(const string &instLine, int currentPC) {
        Instruction instr;
        instr.fullLine = instLine;
        instr.PC = currentPC;
        vector<string> tokens = split(instLine);
        if (tokens.empty()) return;
        instr.mnemonic = tokens[0];
        if (instr.mnemonic == "beq" || instr.mnemonic == "bne") {
            instr.type = InstrType::BRANCH_TYPE;
            if (tokens.size() >= 4) {
                instr.rs = getRegisterNumber(tokens[1]);
                instr.rt = getRegisterNumber(tokens[2]);
                instr.labelTarget = removeComma(tokens[3]);
            }
        }
        else if (instr.mnemonic == "lw" || instr.mnemonic == "sw") {
            instr.type = InstrType::MEM_TYPE;
            if (tokens.size() >= 3) {
                if (instr.mnemonic == "lw")
                    instr.rd = getRegisterNumber(tokens[1]);
                else
                    instr.rd = getRegisterNumber(tokens[1]);
                parseMemoryOperand(tokens[2], instr);
            }
        }
        else if (instr.mnemonic == "sll" || instr.mnemonic == "srl") {
            instr.type = InstrType::SHIFT_TYPE;
            if (tokens.size() >= 4) {
                instr.rd = getRegisterNumber(tokens[1]);
                instr.rt = getRegisterNumber(tokens[2]);
                instr.immediate = parseNumber(tokens[3]);
            }
        }
        else if (instr.mnemonic == "add" || instr.mnemonic == "addu" ||
                 instr.mnemonic == "sub" || instr.mnemonic == "subu" ||
                 instr.mnemonic == "and" || instr.mnemonic == "or" ||
                 instr.mnemonic == "nor" || instr.mnemonic == "slt" || instr.mnemonic == "sltu") {
            instr.type = InstrType::R_TYPE;
            if (tokens.size() >= 4) {
                instr.rd = getRegisterNumber(tokens[1]);
                instr.rs = getRegisterNumber(tokens[2]);
                instr.rt = getRegisterNumber(tokens[3]);
            }
        }
        else if (instr.mnemonic == "addi" || instr.mnemonic == "addiu" ||
                 instr.mnemonic == "andi" || instr.mnemonic == "ori" ||
                 instr.mnemonic == "slti" || instr.mnemonic == "sltiu") {
            instr.type = InstrType::I_TYPE;
            if (tokens.size() >= 4) {
                instr.rd = getRegisterNumber(tokens[1]);
                instr.rs = getRegisterNumber(tokens[2]);
                instr.immediate = convertImmediate(tokens[3]);
            }
        }
        else {
            instr.type = InstrType::I_TYPE;
        }
        instructions.push_back(instr);
        cout << "Parsed instruction: " << instr.fullLine << endl;
    }

    void parseMemoryOperand(const string &operand, Instruction &instr) {
        size_t lparen = operand.find('(');
        size_t rparen = operand.find(')');
        if (lparen != string::npos && rparen != string::npos) {
            string offsetStr = operand.substr(0, lparen);
            string baseStr = operand.substr(lparen + 1, rparen - lparen - 1);
            instr.immediate = convertImmediate(trim(offsetStr));
            int base = getRegisterNumber(trim(baseStr));
            if (instr.mnemonic == "lw")
                instr.rs = base;
            else
                instr.rt = base;
        }
    }

    int getLabelAddress(const string &name) {
        for (auto &lab : labels)
            if (lab.name == name)
                return lab.address;
        return -1;
    }

    //---------------------------
    // Forwarding helper – επιστρέφει την τιμή που πρέπει να χρησιμοποιηθεί για έναν καταχωρητή,
    // ελέγχοντας αν υπάρχει νευραλγική εξαγωγή (forward) από MEM ή WB
    //---------------------------
    int forwardOperand(int regNum, int origVal) {
        // Έλεγχος από MEM στάδιο:
        if (MEM_reg.valid && MEM_reg.instr.mnemonic != "NOP" && MEM_reg.instr.rd == regNum) {
            forwardingFlag = true;
            EX_reg.ForwardingUsed = true;
            return MEM_reg.aluResult;
        }
        // Έλεγχος από WB στάδιο:
        if (WB_reg.valid && WB_reg.instr.mnemonic != "NOP" && WB_reg.instr.rd == regNum) {
            forwardingFlag = true;
            EX_reg.ForwardingUsed = true;
            return WB_reg.aluResult;
        }
        return origVal;
    }

    //---------------------------
    // EX_stage: εκτέλεση ALU λειτουργιών με ενσωματωμένο forwarding
    //---------------------------
    void EX_stage() {
        if (!EX_reg.valid || EX_reg.instr.mnemonic == "NOP") return;
        Instruction &instr = EX_reg.instr;
        int aluResult = 0;
        // Επαναφορά flag forwarding για αυτόν τον κύκλο
        forwardingFlag = false;

        // Για R-TYPE εντολές χρησιμοποιούμε forwarding για τα rs και rt:
        if (instr.mnemonic == "add") {
            int op1 = forwardOperand(instr.rs, cpu.registers[instr.rs]);
            int op2 = forwardOperand(instr.rt, cpu.registers[instr.rt]);
            aluResult = op1 + op2;
            cpu.registers[EX_reg.instr.rd] = aluResult;
            ALU_RESULT=aluResult;
        }
        else if (instr.mnemonic == "addi") {
            int op1 = forwardOperand(instr.rs, cpu.registers[instr.rs]);
            aluResult = op1 + instr.immediate;
            ALU_RESULT=aluResult;
            cpu.registers[EX_reg.instr.rd] = aluResult;
        }
        else if (instr.mnemonic == "addiu") {
            unsigned int u1 = static_cast<unsigned int>(forwardOperand(instr.rs, cpu.registers[instr.rs]));
            unsigned int u2 = static_cast<unsigned int>(instr.immediate);
            aluResult = u1 + u2;
            ALU_RESULT=aluResult;
        }
        else if (instr.mnemonic == "addu") {
            unsigned int u1 = static_cast<unsigned int>(forwardOperand(instr.rs, cpu.registers[instr.rs]));
            unsigned int u2 = static_cast<unsigned int>(forwardOperand(instr.rt, cpu.registers[instr.rt]));
            aluResult = u1 + u2;
            ALU_RESULT=aluResult;
        }
        else if (instr.mnemonic == "sub") {
            int op1 = forwardOperand(instr.rs, cpu.registers[instr.rs]);
            int op2 = forwardOperand(instr.rt, cpu.registers[instr.rt]);
            aluResult = op1 - op2;
            cpu.registers[EX_reg.instr.rd] = aluResult;
            ALU_RESULT=aluResult;
        }
        else if (instr.mnemonic == "subu") {
            unsigned int u1 = static_cast<unsigned int>(forwardOperand(instr.rs, cpu.registers[instr.rs]));
            unsigned int u2 = static_cast<unsigned int>(forwardOperand(instr.rt, cpu.registers[instr.rt]));
            aluResult = u1 - u2;
            ALU_RESULT=aluResult;
        }
        else if (instr.mnemonic == "and") {
            int op1 = forwardOperand(instr.rs, cpu.registers[instr.rs]);
            int op2 = forwardOperand(instr.rt, cpu.registers[instr.rt]);
            aluResult = op1 & op2;
            cpu.registers[EX_reg.instr.rd] = aluResult;
            ALU_RESULT=aluResult;
        }
        else if (instr.mnemonic == "andi") {
            int op1 = forwardOperand(instr.rs, cpu.registers[instr.rs]);
            aluResult = op1 & instr.immediate;
            ALU_RESULT=aluResult;
        }
        else if (instr.mnemonic == "or") {
            int op1 = forwardOperand(instr.rs, cpu.registers[instr.rs]);
            int op2 = forwardOperand(instr.rt, cpu.registers[instr.rt]);
            aluResult = op1 | op2;
            cpu.registers[EX_reg.instr.rd] = aluResult;
            ALU_RESULT=aluResult;
              // Αποθήκευση αποτελέσματος στο EX_reg
        EX_reg.aluResult = aluResult;
        exMonitor.monitor18 = formatMonitorField(aluResult);
        }
        else if (instr.mnemonic == "ori") {
            int op1 = forwardOperand(instr.rs, cpu.registers[instr.rs]);
            aluResult = op1 | instr.immediate;
            ALU_RESULT=aluResult;
        }
        else if (instr.mnemonic == "nor") {
            int op1 = forwardOperand(instr.rs, cpu.registers[instr.rs]);
            int op2 = forwardOperand(instr.rt, cpu.registers[instr.rt]);
            aluResult = ~(op1 | op2);
            ALU_RESULT=aluResult;
        }
        else if (instr.mnemonic == "slt") {
            unsigned int u1 = static_cast<unsigned int>(forwardOperand(instr.rs, cpu.registers[instr.rs]));
            unsigned int u2 = static_cast<unsigned int>(forwardOperand(instr.rt, cpu.registers[instr.rt]));
            aluResult = (u1 < u2) ? 1 : 0;
            ALU_RESULT=aluResult;
        }
        else if (instr.mnemonic == "sll") {
            int op = forwardOperand(instr.rt, cpu.registers[instr.rt]);
            aluResult = op << instr.immediate;
            ALU_RESULT=aluResult;
        }
        else if (instr.mnemonic == "srl") {
            int op = forwardOperand(instr.rt, cpu.registers[instr.rt]);
            aluResult = op >> instr.immediate;
            ALU_RESULT=aluResult;
        }
        else if (instr.mnemonic == "lw") {
            int op1 = forwardOperand(instr.rs, cpu.registers[instr.rs]);
            aluResult = op1 + instr.immediate;
            ALU_RESULT=aluResult;
        }
        else if (instr.mnemonic == "sw") {
            int op1 = forwardOperand(instr.rt, cpu.registers[instr.rt]);
            aluResult = op1 + instr.immediate;
            ALU_RESULT=aluResult;
        }
        // Αποθήκευση αποτελέσματος στο EX_reg
        EX_reg.aluResult = aluResult;
        exMonitor.monitor18 = formatMonitorField(aluResult);
    }

    //---------------------------
    // MEM_stage: για εντολές μνήμης (lw/sw)
    //---------------------------
    void MEM_stage() {
        if (!MEM_reg.valid || MEM_reg.instr.mnemonic == "NOP") return;
        Instruction &instr = MEM_reg.instr;
        if (instr.mnemonic == "lw") {
            MEM_reg.aluResult = memory.read(MEM_reg.aluResult);
        }
        else if (instr.mnemonic == "sw") {
            memory.write(MEM_reg.aluResult, cpu.registers[instr.rd]);
        }
    }

    //---------------------------
    // WB_stage: write-back στο register file
    //---------------------------
    void WB_stage() {
        if (!WB_reg.valid || WB_reg.instr.mnemonic == "NOP") return;
        Instruction &instr = WB_reg.instr;
        // Δεν γράφουμε πίσω για sw, j, beq, bne
        if (instr.mnemonic != "sw" && instr.mnemonic != "j" &&
            instr.mnemonic != "beq" && instr.mnemonic != "bne") {
            cpu.registers[instr.rd] = WB_reg.aluResult;
        }
        cpu.registersfinal[instr.rd] = WB_reg.aluResult;
    }

    //---------------------------
    // ID_stage: ανάλυση εντολής, branch resolution & hazard detection (συμπεριλαμβανομένης της ανίχνευσης branch hazards)
    //---------------------------
    bool ID_stage() {
        if (!ID_reg.valid || ID_reg.instr.mnemonic == "NOP")
            return false;
        Instruction &instr = ID_reg.instr;

        // Branch hazard detection: αγνοούμε τις εξαρτήσεις αν ο register είναι $zero (32)
        if ((instr.mnemonic == "beq" || instr.mnemonic == "bne")) {
            // Έλεγχος στο EX στάδιο
            if (EX_reg.valid && EX_reg.instr.mnemonic != "NOP") {
                if (((instr.rs != 32) && (instr.rs == EX_reg.instr.rd)) ||
                    ((instr.rt != 32) && (instr.rt == EX_reg.instr.rd))) {
                    hazardFlag = true;
                    ID_reg.HazardTriggered = true;
                    return true; // Stall λόγω branch hazard από EX
                }
            }
            // Έλεγχος στο MEM στάδιο
            if (MEM_reg.valid && MEM_reg.instr.mnemonic != "NOP") {
                if (((instr.rs != 32) && (instr.rs == MEM_reg.instr.rd)) ||
                    ((instr.rt != 32) && (instr.rt == MEM_reg.instr.rd))) {
                    hazardFlag = true;
                    ID_reg.HazardTriggered = true;
                    return true; // Stall λόγω branch hazard από MEM
                }
            }
        }

        // Branch resolution
        if (instr.mnemonic == "beq") {
            if (cpu.registers[instr.rs] == cpu.registers[instr.rt]) {
                int target = getLabelAddress(instr.labelTarget);
                if (target != -1) {
                    branchTakenFlag = true;
                    branchTargetAddress = target;
                }
            }
        }
        else if (instr.mnemonic == "bne") {
            if (cpu.registers[instr.rs] != cpu.registers[instr.rt]) {
                int target = getLabelAddress(instr.labelTarget);
                if (target != -1) {
                    branchTakenFlag = true;
                    branchTargetAddress = target;
                }
            }
        }

        // Hazard detection για load-use hazard (αγνοώντας και εδώ $zero)
        hazardFlag = false;
        if (EX_reg.valid && EX_reg.instr.mnemonic == "lw") {
            int lwDest = EX_reg.instr.rd;
            if (((instr.rs != 32) && (instr.rs == lwDest)) ||
                ((instr.rt != 32) && (instr.rt == lwDest))) {
                hazardFlag = true;
                ID_reg.HazardTriggered = true;
                return true; // Stall λόγω load-use hazard
            }
        }
        return branchTakenFlag;
    }


    //---------------------------
    // IF_stage: fetch εντολής βάσει του pc
    //---------------------------
    void IF_stage() {
        int index = pc / 4;
        if (index < (int)instructions.size()) {
            IF_reg.instr = instructions[index];
            IF_reg.instr.PC = pc - 4;
            IF_reg.valid = true;
            pc += 4;
        }
        else {
            IF_reg.instr = createBubble();
            IF_reg.valid = false;
        }

    }

    // Συνάρτηση μορφοποίησης για monitors
    string formatMonitorField(int value) {
        return toHex(value);
    }
    string formatMonitorField(const string &value) {
        return value;
    }

    vector<string> computeUnifiedMonitors() {
        // Δημιουργούμε vector με 28 θέσεις, αρχικοποιημένες ως "-"
        vector<string> mon(28, "-");

        // ------------------------------
        // IF stage (θέσεις 0-2)
        // ------------------------------
        // Monitor 1: Εάν η εντολή στο ID είναι branch, εμφανίζεται το label target, αλλιώς η τιμή του pc
        mon[0] = (ID_reg.instr.mnemonic == "beq" || ID_reg.instr.mnemonic == "bne") ?
                 formatMonitorField(ID_reg.instr.labelTarget) : formatMonitorField(pc);
        // Monitor 2: Τρέχον pc (pc-4)
        mon[1] = formatMonitorField(pc - 4);
        // Monitor 3: Ολόκληρη η γραμμή εντολής από το IF
        mon[2] = formatMonitorField(IF_reg.instr.fullLine);

        // ------------------------------
        // ID stage (θέσεις 3-14)
        // ------------------------------
        if (ID_reg.valid && ID_reg.instr.mnemonic != "NOP") {
            // Monitor 4: Για branch εμφανίζεται το label target, αλλιώς "-"
            mon[3] = (ID_reg.instr.mnemonic == "beq" || ID_reg.instr.mnemonic == "bne") ?
                     formatMonitorField(ID_reg.instr.labelTarget) : "-";

            // Διακρίνουμε τα είδη εντολών στο ID:
            if (ID_reg.instr.type == InstrType::R_TYPE || ID_reg.instr.type == InstrType::SHIFT_TYPE) {
                mon[4]  = getRegisterName(ID_reg.instr.rs);         // Monitor 5
                mon[5]  = getRegisterName(ID_reg.instr.rt);         // Monitor 6
                // Monitor 7 & 8: Αν η WB έχει branch δεν εμφανίζουμε τιμές, αλλιώς
                if (WB_reg.instr.type == InstrType::BRANCH_TYPE) {
                    mon[6] = "-";                                  // Monitor 7
                    mon[7] = "-";                                  // Monitor 8
                } else {
                    mon[6] = getRegisterName(WB_reg.instr.rd);       // Monitor 7
                    mon[7] = formatMonitorField(WB_reg.aluResult);   // Monitor 8
                }
                mon[8]  = formatMonitorField(cpu.registers[ID_reg.instr.rs]);  // Monitor 9
                if(ID_reg.instr.mnemonic=="slt")
                mon[9]  = formatMonitorField(cpu.registers[ID_reg.instr.rs]);  // Monitor 10
                else
                mon[9]  = formatMonitorField(cpu.registers[ID_reg.instr.rt]);  // Monitor 10
                mon[10] = "-";                                             // Monitor 11
                mon[11] = getRegisterName(ID_reg.instr.rs);                  // Monitor 12
                mon[12] = getRegisterName(ID_reg.instr.rt);                  // Monitor 13
                mon[13] = getRegisterName(ID_reg.instr.rt);                  // Monitor 14
                mon[14] = getRegisterName(ID_reg.instr.rd);                  // Monitor 15
            }
            else if (ID_reg.instr.type == InstrType::I_TYPE) {
                mon[4]  = getRegisterName(ID_reg.instr.rs);         // Monitor 5
                mon[5]  = "-";                                     // Monitor 6
                // Σε περίπτωση I-type, ελέγχουμε την κατάσταση της WB για monitors 7 και 8
                if (WB_reg.instr.fullLine == "NOP")
                {
                    mon[6] = mon[24];                       // Monitor 7
                    mon[7] = mon[25];                       // Monitor 8
                }
                else {
                    mon[6] = getRegisterName(WB_reg.instr.rd);       // Monitor 7
                    mon[7] = formatMonitorField(WB_reg.aluResult);   // Monitor 8
                }
                if(ID_reg.instr.mnemonic=="addi")
                mon[8]  = formatMonitorField(cpu.registers[ID_reg.instr.rs] + ID_reg.instr.immediate);  // Monitor 9
            else if(ID_reg.instr.mnemonic=="ori")
                mon[8]  = formatMonitorField(cpu.registers[ID_reg.instr.rs]);  // Monitor 9 (τροποποιημένο)
            else
                mon[8]  = formatMonitorField(cpu.registers[ID_reg.instr.rs]);  // Monitor 9


                mon[9]  = "-";                                     // Monitor 10
                mon[10] = formatMonitorField(ID_reg.instr.immediate);          // Monitor 11
                ID_reg.monitoritype11=mon[10];
                mon[11] = getRegisterName(ID_reg.instr.rs);                      // Monitor 12
                mon[12] = getRegisterName(ID_reg.instr.rd);                      // Monitor 13
                mon[13] = getRegisterName(ID_reg.instr.rd);                      // Monitor 14
                mon[14] = "-";                                                 // Monitor 15
            }
            else if (ID_reg.instr.type == InstrType::BRANCH_TYPE) {
                mon[4]  = getRegisterName(ID_reg.instr.rt);         // Monitor 5
                mon[5]  = getRegisterName(ID_reg.instr.rt);         // Monitor 6

                mon[6]  = getRegisterName(ID_reg.instr.rt);         // Monitor 7
                mon[7]  = formatMonitorField(cpu.registers[ID_reg.instr.rt]);  // Monitor 8
                mon[8]  = formatMonitorField(cpu.registers[ID_reg.instr.rs]);  // Monitor 9
                mon[9]  = formatMonitorField(cpu.registers[ID_reg.instr.rt]);  // Monitor 10
                mon[10] = "-";                                     // Monitor 11                                // Monitor 9
                mon[11]  = getRegisterName(ID_reg.instr.rt);         // Monitor 12
                mon[12] = getRegisterName(ID_reg.instr.rs);         // Monitor 13
                mon[13] = getRegisterName(ID_reg.instr.rs);         // Monitor 14
                mon[14] = "-";                                     // Monitor 15


            }
            else if (ID_reg.instr.type == InstrType::MEM_TYPE) {
                if (ID_reg.instr.mnemonic == "lw") {
                    mon[4]  = getRegisterName(ID_reg.instr.rs);         // Monitor 5
                    mon[5]  = getRegisterName(ID_reg.instr.rd);         // Monitor 6
                    mon[6]  = "-";                                     // Monitor 7
                    mon[7]  = "-";                                     // Monitor 8
                    mon[8]  = formatMonitorField(cpu.registersfinal[ID_reg.instr.rs]); // Monitor 9
                    mon[9]  = "-";                                     // Monitor 10
                    mon[10] = formatMonitorField(ID_reg.instr.immediate);          // Monitor 11
                    mon[11] = getRegisterName(ID_reg.instr.rs);                      // Monitor 12
                    mon[12] = getRegisterName(ID_reg.instr.rd);                      // Monitor 13
                    mon[13] =  getRegisterName(ID_reg.instr.rd);   // Monitor 14
                    mon[14] = "-";                                                 // Monitor 15
                } else { // περίπτωση "sw"
                    mon[4]  = getRegisterName(ID_reg.instr.rt);         // Monitor 5
                    mon[5]  = getRegisterName(ID_reg.instr.rd);         // Monitor 6
                    if (WB_reg.instr.type == InstrType::BRANCH_TYPE) {
                        mon[6]  = "-";                                // Monitor 7
                        mon[7]  = "-";                                // Monitor 8
                    } else {
                        mon[6]  = getRegisterName(WB_reg.instr.rd);    // Monitor 7
                        mon[7]  = formatMonitorField(WB_reg.aluResult); // Monitor 8
                    }
                    mon[8]  = formatMonitorField(cpu.registersfinal[ID_reg.instr.rt]); // Monitor 9
                    mon[9]  = formatMonitorField(cpu.registersfinal[ID_reg.instr.rd]); // Monitor 10
                    mon[10] = formatMonitorField(ID_reg.instr.immediate);              // Monitor 11
                    mon[11] = getRegisterName(ID_reg.instr.rt);                        // Monitor 12
                    mon[12] = getRegisterName(ID_reg.instr.rd);                        // Monitor 13
                    mon[13] = getRegisterName(ID_reg.instr.rd);                        // Monitor 14
                    mon[14] = "-";                                                     // Monitor 15
                }
            }
        }
        else {
            // Αν δεν υπάρχει έγκυρη εντολή στο ID, γεμίζουμε θέσεις 3 έως 14 με "-"
            for (int i = 3; i < 15; i++)
                mon[i] = "-";
        }

        // ------------------------------
        // EX stage (θέσεις 15-18)
        // ------------------------------
        if (EX_reg.valid && EX_reg.instr.mnemonic != "NOP") {
            if (EX_reg.instr.type == InstrType::R_TYPE || EX_reg.instr.type == InstrType::SHIFT_TYPE) {
                mon[15] = formatMonitorField(cpu.registers[EX_reg.instr.rs]);  // Monitor 16
                mon[16] = formatMonitorField(cpu.registers[EX_reg.instr.rt]);  // Monitor 17
                mon[17] = formatMonitorField(EX_reg.aluResult);                // Monitor 18
                mon[18] = "-";                                                 // Monitor 19
            }
            else if (EX_reg.instr.type == InstrType::I_TYPE) {
                mon[15] = formatMonitorField(EX_reg.aluResult);  // Monitor 16

                mon[16] = formatMonitorField(EX_reg.instr.immediate);          // Monitor 17
                mon[17] = formatMonitorField( EX_reg.aluResult);           // Monitor 18
                mon[18] = "-";                                                 // Monitor 19
            }
            else if (EX_reg.instr.type == InstrType::BRANCH_TYPE) {
                // Για branch, όλα τα monitors του EX παραμένουν "-"
                for (int i = 15; i <= 18; i++)
                    mon[i] = "-";
            }
            else if (EX_reg.instr.type == InstrType::MEM_TYPE) {
                if (EX_reg.instr.mnemonic == "lw") {
                    string sum20 = idMonitor.monitor9 + formatMonitorField(EX_reg.instr.immediate);
                    mon[15] =  formatMonitorField(cpu.registers[EX_reg.instr.rs]);                              // Monitor 16
                    exMonitor.monitor16 = formatMonitorField(cpu.registers[EX_reg.instr.rd]);
                    mon[16] = formatMonitorField(EX_reg.instr.immediate);        // Monitor 17
                    mon[17] =  formatMonitorField(cpu.registers[EX_reg.instr.rs]); // Monitor 18
                    mon[18] = "-";                                               // Monitor 19
                } else { // περίπτωση "sw"
                    mon[15] = idMonitor.monitor9;                              // Monitor 16
                    mon[16] = formatMonitorField(EX_reg.instr.immediate);        // Monitor 17
                    mon[17] = formatMonitorField(EX_reg.instr.immediate);        // Monitor 18
                    mon[18] = exMonitor.monitor16;                             // Monitor 19
                }
            }
        }
        else {
            for (int i = 15; i < 19; i++)
                mon[i] = "-";
        }

        // ------------------------------
        // MEM stage (θέσεις 19-23)
        // ------------------------------
        if (MEM_reg.valid && MEM_reg.instr.mnemonic != "NOP") {
            if (MEM_reg.instr.type == InstrType::BRANCH_TYPE) {
                // Για branch, διατηρούμε τις τιμές ως "-"
            }
            else if (MEM_reg.instr.type == InstrType::R_TYPE || MEM_reg.instr.type == InstrType::SHIFT_TYPE) {
                mon[19] = "-";                                    // Monitor 20
                mon[20] = "-";                                    // Monitor 21
                mon[21] = "-";                                    // Monitor 22
                mon[22] = formatMonitorField(cpu.registers[MEM_reg.instr.rs]); // Monitor 23
                mon[23] = getRegisterName(MEM_reg.instr.rd);       // Monitor 24
            }
            else if (MEM_reg.instr.type == InstrType::I_TYPE) {
                mon[19] = "-";
                mon[20] = "-";
                mon[21] = "-";

               // if(ID_reg.instr.type==InstrType::I_TYPE)
                //mon[22]=MEM_reg.monitoritype11; // Monitor 23
               // else
               mon[22] = formatMonitorField(cpu.registers[MEM_reg.instr.rs]); // Monitor 23

                mon[23] = getRegisterName(MEM_reg.instr.rd);      // Monitor 24
            }
            else if (MEM_reg.instr.type == InstrType::MEM_TYPE) {
                if (MEM_reg.instr.mnemonic == "lw") {
                    mon[19] =  formatMonitorField(cpu.registers[MEM_reg.instr.rs]);               // Monitor 20
                    mon[20] = "-";                                // Monitor 21
                    mon[21] =mon[9]; // Monitor 22
                    mon[22] = "-";                                // Monitor 23
                    mon[23] = getRegisterName(MEM_reg.instr.rd);    // Monitor 24
                }
                else { // περίπτωση "sw"
                    mon[19] = formatMonitorField(cpu.registers[MEM_reg.instr.rt]);  // Monitor 20
                    mon[20] = formatMonitorField(cpu.registers[MEM_reg.instr.rd]); // Monitor 21
                    // Τα monitors 22-24 παραμένουν "-"
                }
            }
        }
        else {
            for (int i = 19; i < 24; i++)
                mon[i] = "-";
        }

        // ------------------------------
        // WB stage (θέσεις 24-27)
        // ------------------------------
        if (WB_reg.valid && WB_reg.instr.mnemonic != "NOP") {
            if (WB_reg.instr.type == InstrType::BRANCH_TYPE || WB_reg.instr.mnemonic == "sw") {
                // Σε περίπτωση branch ή sw:
                mon[24] = "-";
                mon[25] = "-";
                mon[26] = (ID_reg.HazardTriggered ? "1" : "0");      // Monitor 27
                mon[27] = (EX_reg.ForwardingUsed ? "1" : "0");       // Monitor 28
            }
            else {
                mon[24] = getRegisterName(WB_reg.instr.rd);         // Monitor 25
                mon[25] = formatMonitorField(WB_reg.aluResult);      // Monitor 26
                mon[26] = (ID_reg.HazardTriggered ? "1" : "0");      // Monitor 27
                mon[27] = (EX_reg.ForwardingUsed ? "1" : "0");       // Monitor 28
            }
        }
        else {
            for (int i = 24; i < 28; i++)
                mon[i] = "-";
            mon[26] = (ID_reg.HazardTriggered ? "1" : "0");
            mon[27] = (EX_reg.ForwardingUsed ? "1" : "0");
        }




        if (WB_reg.instr.type == InstrType::BRANCH_TYPE || WB_reg.instr.mnemonic=="NOP" || ID_reg.instr.mnemonic=="NOP" ) {
            mon[6] = "-";                                  // Monitor 7
            mon[7] = "-";                                  // Monitor 8
        } else {
            mon[6] = getRegisterName(WB_reg.instr.rd);       // Monitor 7
            mon[7] = formatMonitorField(WB_reg.aluResult);   // Monitor 8
        }

        if(MEM_reg.instr.type ==InstrType::R_TYPE && ID_reg.instr.type==InstrType::I_TYPE)
         mon[22]=mon[25];
        return mon;
    }

    


    void printAllMonitors(ofstream &out)
    {
        vector<string> allMon = computeUnifiedMonitors();

        for (auto &s : allMon) out << s << "\t";
    }


    // Μία μικρή συνάρτηση για να μετατρέπει τυχόν "NOP" γραμμές σε "-"
    void pipelineformat() {
        if (IF_reg.instr.fullLine == "NOP") IF_reg.instr.fullLine = "Bubble";
        if (ID_reg.instr.fullLine == "NOP") ID_reg.instr.fullLine = "Bubble";
        if (EX_reg.instr.fullLine == "NOP") EX_reg.instr.fullLine = "Bubble";
        if (MEM_reg.instr.fullLine == "NOP") MEM_reg.instr.fullLine = "Bubble";
        if (WB_reg.instr.fullLine == "NOP") WB_reg.instr.fullLine = "Bubble";
    }

    //---------------------------
    // Print state του τρέχοντος κύκλου
    //---------------------------
    void printCycleState(int cycle, ofstream &out) {
        out << "-----Cycle " << cycle << "-----\n";
        out << "Registers:\n";
        int printedPC = pc - 4;
        out << toHex(printedPC) << "\t";
        for (int i = 0; i < cpu.registersfinal.size() - 1; i++) {
            out << toHex(cpu.registersfinal[i]) << "\t";
        }
        out << "\n\n";
        out << "Monitors:\n";
        printAllMonitors(out);
        out << "\n\n";
        out << "Memory State:\n" << memory.getMemoryState() << "\n\n";
        pipelineformat();
        out << "Pipeline Stages:\n";
        out << IF_reg.instr.fullLine << "\t" << ID_reg.instr.fullLine << "\t"
            << EX_reg.instr.fullLine << "\t" << MEM_reg.instr.fullLine << "\t"
            << WB_reg.instr.fullLine << "\n\n";
    }

public:
    Simulator() : pc(0), cycleCount(1) { }

    void loadInstructions(const string &filename) {
        ifstream fin(filename);
        if (!fin) {
            cerr << "Cannot open file: " << filename << endl;
            exit(1);
        }
        string line;
        int currentPC = 0;
        while (getline(fin, line)) {
            int before = instructions.size();
            parseLine(line, currentPC);
            int after = instructions.size();
            if (after > before)
                currentPC += 4;
        }
        fin.close();
    }

    // run: η κύρια λούπα του simulation με ενσωματωμένο hazard detection, branch hazard handling & forwarding
    void run(const vector<int> &printCycles, const string &outputFile,
             const string &studentName, const string &studentID, clock_t &startclock, clock_t endclock) {
        ofstream fout(outputFile);
        if (!fout) {
            cerr << "Cannot create output file: " << outputFile << endl;
            exit(1);
        }
        fout << "Name: " << studentName << "\n";
        fout << "University ID: " << studentID << "\n\n";

        // Αρχικοποίηση των pipeline registers ως bubbles
        IF_reg = PipelineRegister();
        ID_reg = PipelineRegister();
        EX_reg = PipelineRegister();
        MEM_reg = PipelineRegister();
        WB_reg = PipelineRegister();

        // Fetch αρχικής εντολής στο IF
        IF_stage();
        cycleCount++;

        // Κύρια λούπα εκτέλεσης μέχρι να αδειάσει το pipeline ή να εντοπιστεί "sll $zero, $zero, 0"
        while ((IF_reg.valid || ID_reg.valid || EX_reg.valid || MEM_reg.valid || WB_reg.valid) && (sllflagtelos == -1)) {
            WB_stage();
            MEM_stage();
            EX_stage();
            bool branchOrHazard = ID_stage();


            // Ενσωμάτωση hazard detection: εάν εντοπιστεί load-use ή branch hazard, δεν προωθούμε τα IF και ID
            if (hazardFlag) {
                // Εισαγωγή bubble στο EX στάδιο και διατήρηση της ID_reg (stall)
                PipelineRegister bubble;
                bubble.instr = createBubble();
                bubble.valid = false;
                WB_reg = MEM_reg;
                MEM_reg = EX_reg;
                EX_reg = bubble;
                // Δεν μετακινούμε το ID_reg και το IF_reg ώστε να γίνει stall
            } else {
                // Shift κανονικά τα pipeline registers
                WB_reg = MEM_reg;
                MEM_reg = EX_reg;
                EX_reg = ID_reg;
                ID_reg = IF_reg;
                IF_stage();
            }



            // Εάν έχει γίνει branch, flush τα IF και ID registers πριν συνεχίσει το fetch
            if (branchTakenFlag) {
                IF_reg = PipelineRegister();
                ID_reg = PipelineRegister();
                pc = branchTargetAddress;
                IF_stage();
                branchTakenFlag = false;
            }


            // Εκτύπωση κύκλου αν ο κύκλος ανήκει στις ζητούμενες τιμές
         if (find(printCycles.begin(), printCycles.end(), cycleCount) != printCycles.end()) {
                WB_stage();
                printCycleState(cycleCount, fout);
            }

            // Έλεγχος τερματισμού: εάν στο WB υπάρχει "sll $zero, $zero, 0"
            if (WB_reg.instr.mnemonic == "sll" &&
                WB_reg.instr.rd == getRegisterNumber("zero") &&
                WB_reg.instr.rt == getRegisterNumber("zero") &&
                WB_reg.instr.immediate == 0) {
                sllflagtelos = 1;
            }
            cycleCount++;
        }
        printFinalState(fout);
        endclock = clock();
        double duration = double(endclock - startclock) / CLOCKS_PER_SEC;
        double duration_in_ns = duration * 1.0e9;
        fout << duration_in_ns << "ns" << endl;
        fout.close();
    }

    void printFinalState(ofstream &out) {
        out << "-----Final State-----\n";
        out << "Registers:\n";
        int printedPC = (cycleCount - 1) * 4;
        out << toHex(pc - 4) << "\t";
        for (int i = 0; i < 32; i++) {
            out << toHex(cpu.registers[i]) << "\t";
        }
        out << "\n\nMemory State:\n" << memory.getMemoryState() << "\n\n";
        out << "Total Cycles:\n" << (cycleCount - 1) << "\n\n";
        out << "User Time:\n";
        out<<endl;
    }
};

//
// main: ανάγνωση εισόδου χρήστη και εκτέλεση του simulation
//
int main(){
    clock_t startclock, endclock;
    int numPrintCycles;
    cout << "Enter the number of cycles you want to print: ";
    cin >> numPrintCycles;
    vector<int> printCycles(numPrintCycles);
    cout << "Enter the cycle numbers (separated by spaces): ";
    for (int i = 0; i < numPrintCycles; i++){
        cin >> printCycles[i];
    }
    startclock = clock();
    string studentName = "Iosif Nicolaou";
    string studentID = "UC10xxxxx";
    Simulator sim;
    sim.loadInstructions("pipeline_test2025.txt");
    sim.run(printCycles, "Iosif_Nicolaou_UC10xxxxx_lab4.txt", studentName, studentID, startclock, endclock);
    cout << "Simulation complete.\n";
    return 0;
}
