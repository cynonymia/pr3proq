#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <sstream>
#include "circuit.hpp"
#include "globals.hpp"
#include "logging.hpp"
#include "qcir.hpp"
#include "preproq.hpp"
#include "writer.hpp"

#define INPUT(x) FILE* input = fmemopen((char*)x, strlen(x), "r")

#define CIRCUIT(x) Circuit circ; do {INPUT(x); REQUIRE(qcir::parse(input, circ) == PREPROQ_OK); fclose(input);} while(0)

using namespace preproq;

#define PREFIX "[Test] "

TEST_CASE("Simple parse cleansed", "[QCIR]"){    
  CIRCUIT("#QCIR-14\n" "forall(1)\n" "exists(2)\n" "output(-5)\n" "3 = or(1, 2)\n" "4 = or(-1, -2)\n" "5 = and(3, 4)\n");

  REQUIRE(circ.varSize() == 5);  
  REQUIRE(circ.root == -5);

  bool tseitin[5] = {false, false, true, true, true};
  unsigned char assignment[5] = {VA_None, VA_None, VA_None, VA_None, VA_None};
  unsigned char gtype[5] = {0, 0, GT_Or, GT_Or, GT_And};
  size_t head[5] = {0, 0, 1, 4, 7};
  unsigned char qtype[5] = {Forall, Exists, Tseitin, Tseitin, Tseitin};

  for(VarId vid = 1; vid <= 5; vid++) {
    PDBG("Testing properties for " << vid);
    REQUIRE(circ.tseitin(vid) == tseitin[vid-1]);
    REQUIRE(circ.var(vid).assignment == assignment[vid-1]);
    REQUIRE(circ.var(vid).gtype == gtype[vid-1]);
    REQUIRE(circ.var(vid).head == head[vid-1]);
    REQUIRE(circ.var(vid).qtype == qtype[vid-1]);
  }  

  Literal expected[9] = {1,2,0,-1,-2,0,3,4,0};

  for(NodeChild cid = 1; cid <= 9; cid++) {
    PDBG("Testing properties for " << cid);
    REQUIRE(circ.get(cid) == expected[cid-1]);    
  }
}

TEST_CASE("Simple parse", "[QCIR]"){
    
  INPUT("#QCIR-14\n" "forall(a)\n" "exists(b)\n" "output(-g3)\n" "g1 = or(a, b)\n" "g2 = or(-a, -b)\n" "g3 = and(g1, g2)\n");
  Circuit circ;
  REQUIRE(qcir::parse(input, circ) == PREPROQ_OK);
  
  REQUIRE(circ.varSize() == 5);  
  REQUIRE(circ.root == -5);

  bool tseitin[5] = {false, false, true, true, true};
  unsigned char assignment[5] = {VA_None, VA_None, VA_None, VA_None, VA_None};
  unsigned char gtype[5] = {0, 0, GT_Or, GT_Or, GT_And};
  size_t head[5] = {0, 0, 1, 4, 7};
  unsigned char qtype[5] = {Forall, Exists, Tseitin, Tseitin, Tseitin};

  for(VarId vid = 1; vid <= 5; vid++) {
    PDBG("Testing properties for " << vid);
    REQUIRE(circ.tseitin(vid) == tseitin[vid-1]);
    REQUIRE(circ.var(vid).assignment == assignment[vid-1]);
    REQUIRE(circ.var(vid).gtype == gtype[vid-1]);
    REQUIRE(circ.var(vid).head == head[vid-1]);
    REQUIRE(circ.var(vid).qtype == qtype[vid-1]);
  }  

  Literal expected[9] = {1,2,0,-1,-2,0,3,4,0};

  for(NodeChild cid = 1; cid <= 9; cid++) {
    PDBG("Testing properties for " << cid);
    REQUIRE(circ.get(cid) == expected[cid-1]);    
  }
}

TEST_CASE("Parse empty gates", "[QCIR]"){
    
  INPUT("#QCIR-14\n" "forall(a)\n" "exists(b)\n" "output(g3)\n" "g1 = or()\n" "g2 = or()\n" "g3 = and(g1, g2)\n");
  Circuit circ;
  REQUIRE(qcir::parse(input, circ) == PREPROQ_OK);
}


TEST_CASE("Correct Init", "[PreProQ]") {
  CIRCUIT("#QCIR-14\n" "forall(1)\n" "exists(2)\n" "output(-5)\n" "3 = or(1, 2)\n" "4 = or(-1, -2)\n" "5 = and(-3, -4)\n");
  PreProQ p(circ);
  
  bool pos[5] = {true, true, true, true, false};
  bool neg[5] = {true, true, false, false, true};

  for(VarId vid = 1; vid <= 5; vid++) {    
    PDBG("Testing properties for " << vid);
    REQUIRE(circ.var(vid).pos == pos[vid-1]);
    REQUIRE(circ.var(vid).neg == neg[vid-1]);
  }  
}

TEST_CASE("Eliminate empty gate", "[PreProQ]") {
  CIRCUIT("#QCIR-14\n" "exists(1)\n" "output(4)\n" "2 = and()\n" "3 = or()\n" "4 = or(2,3)\n");
  PreProQ p(circ);  
  REQUIRE(p.run() == PREPROQ_SAT);
  REQUIRE(circ.var(2).assignment == VA_True);
  REQUIRE(circ.var(3).assignment == VA_False);
}

TEST_CASE("Eliminate Singularity simple", "[PreProQ]") {
  CIRCUIT("#QCIR-14\n" "exists(1, 2)\n" "output(5)\n" "3 = and(1)\n" "4 = or(-3)\n" "5 = or(2,4)\n");
  //mem: X 1 0 3 0 2 4 0
  //idx: 0 1 2 3 4 5 6 7
  PreProQ p(circ);
  REQUIRE(p.run() == PREPROQ_SAT);
  //REQUIRE(circ.get(6) == -1);
}

TEST_CASE("Eliminate Duplicate/Tautology simple", "[PreProQ]") {
  CIRCUIT("#QCIR-14\n" "exists(1,2,3,4)\n" "output(9)\n" "5 = and(1,1,2)\n" "6 = and(3,-3,4)\n" "7 = or(1,1,2)\n" "8 = or(3,-3,4)\n" "9 = or(5,6,7,8)\n");
  PreProQ p(circ);
  REQUIRE(p.run() == PREPROQ_SAT);
  //  REQUIRE(circ.calculateChildrenCount(5)==2);
  //  REQUIRE(circ.var(6).assignment==VA_False);  
  //  REQUIRE(circ.calculateChildrenCount(7)==2);
  //  REQUIRE(circ.var(8).assignment==VA_True);
}

TEST_CASE("Eliminate Constant", "[PreProQ]") {
    CIRCUIT("#QCIR-14\n" "exists(1,2)\n" "output(4)\n" "3 = or()\n" "4 = and(-3, 1, 2)\n");
    PreProQ p(circ);
    REQUIRE(p.run() == PREPROQ_SAT);
    VarId vid = 4;
    NodeChild c = circ.begin(vid);
    //    REQUIRE(circ.get(c) == 1);
    //    c = circ.next(c);
    //    REQUIRE(circ.get(c) == 2);
    //    c = circ.next(c);
    //    REQUIRE(circ.isEnd(c));
}

TEST_CASE("Gate Collapse", "[PreProQ]") {
    CIRCUIT("#QCIR-14\n" "exists(1,2)\n" "forall(3,4)\n" "output(6)\n" "5 = and(1,2)\n" "6 = and(3, 4, 5)\n");
    PreProQ p(circ);
    REQUIRE(p.run() == PREPROQ_UNSAT);
    VarId vid = 6;
    NodeChild c = circ.begin(vid);
    REQUIRE(circ.get(c) == 3);
    c = circ.next(c);
    REQUIRE(circ.get(c) == 4);
    c = circ.next(c);
    //    REQUIRE(circ.get(c) == 1);
    //    c = circ.next(c);
    //    REQUIRE(circ.get(c) == 2);
    //    c = circ.next(c);
    //    REQUIRE(circ.isEnd(c));
}

TEST_CASE("Gate Collapse Negated", "[PreProQ]") {
    CIRCUIT("#QCIR-14\n" "exists(1,2)\n" "forall(3,4)\n" "output(6)\n" "5 = or(1,2)\n" "6 = and(3, 4, -5)\n");
    PreProQ p(circ);
    REQUIRE(p.run() == PREPROQ_UNSAT);
    VarId vid = 6;
    NodeChild c = circ.begin(vid);
    REQUIRE(circ.get(c) == 3);
    c = circ.next(c);
    REQUIRE(circ.get(c) == 4);
    c = circ.next(c);
    //    REQUIRE(circ.get(c) == -1);
    //    c = circ.next(c);
    //    REQUIRE(circ.get(c) == -2);
    //    c = circ.next(c);
    //    REQUIRE(circ.isEnd(c));
}

TEST_CASE("Local Unit Assignment Condition", "[PreProQ]") {
    GType gtype = GT_Or;
    Literal lit = 1;
    
    REQUIRE(!((gtype == GType::GT_And) == (lit > 0)));
    lit = -1;
    REQUIRE(((gtype == GType::GT_And) == (lit > 0)));
    gtype = GT_And;
    REQUIRE(!((gtype == GType::GT_And) == (lit > 0)));
    lit = 1;
    REQUIRE(((gtype == GType::GT_And) == (lit > 0)));
}

TEST_CASE("Local Unit", "[PreProQ]") {
    CIRCUIT("#QCIR-14\n" "forall(1)\n" "exists(2)\n" "output(4)\n" "3 = or (1, -2)\n" "4 = and (3, 1)\n");
    PreProQ p(circ);
    REQUIRE(p.run() == PREPROQ_UNSAT);
}
