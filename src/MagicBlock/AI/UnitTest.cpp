
#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>

#include "MagicBlock/AI/UnitTest.h"
#include <MagicBlock/AI/MoveSeq.h>
#include "MagicBlock/AI/Board.h"
#include "MagicBlock/AI/Algorithm.h"
#include "MagicBlock/AI/jm_malloc.h"
#include "MagicBlock/AI/SparseBitset.h"

#include "MagicBlock/AI/Console.h"
#include "MagicBlock/AI/CPUWarmUp.h"
#include "MagicBlock/AI/StopWatch.h"

using namespace MagicBlock::AI;
using namespace jm_malloc;

void SparseTrieBitset_test()
{
    MagicBlock::AI::SparseBitset<Board<5, 5>, 3, 25> visited;
    Board<5, 5> board;
    board.cells[0] = Color::Blue;
    board.cells[1] = Color::Red;
    board.cells[2] = Color::Green;
    board.cells[3] = Color::Orange;
    board.cells[4] = Color::White;

    board.cells[5] = Color::Yellow;
    board.cells[6] = Color::Empty;
    board.cells[7] = Color::Red;
    board.cells[8] = Color::Blue;
    board.cells[9] = Color::Unknown;

    board.cells[10] = Color::Red;
    board.cells[11] = Color::Yellow;
    board.cells[12] = Color::Orange;
    board.cells[13] = Color::Blue;
    board.cells[14] = Color::White;

    board.cells[15] = Color::Green;
    board.cells[16] = Color::Yellow;
    board.cells[17] = Color::Red;
    board.cells[18] = Color::Blue;
    board.cells[19] = Color::Orange;

    board.cells[20] = Color::Red;
    board.cells[21] = Color::Yellow;
    board.cells[22] = Color::Orange;
    board.cells[23] = Color::Blue;
    board.cells[24] = Color::White;

    visited.insert(board);
    visited.insert(board);

    visited.shutdown();
}

void MoveSeq_test()
{
    MoveSeq moveSeq;

    moveSeq.push_back(0);
    moveSeq.push_back(1);
    moveSeq.push_back(2);
    moveSeq.push_back(3);
    moveSeq.push_back(4);
    moveSeq.push_back(5);
    moveSeq.push_back(6);
    moveSeq.push_back(7);

    moveSeq.push_back(0);
    moveSeq.push_back(1);
    moveSeq.push_back(2);
    moveSeq.push_back(3);
    moveSeq.push_back(4);
    moveSeq.push_back(5);
    moveSeq.push_back(6);
    moveSeq.push_back(7);

    moveSeq.push_back(0);
    moveSeq.push_back(1);
    moveSeq.push_back(2);
    moveSeq.push_back(3);
    moveSeq.push_back(4);
    moveSeq.push_back(5);
    moveSeq.push_back(6);
    moveSeq.push_back(7);

    moveSeq.push_back(0);
    moveSeq.push_back(1);
    moveSeq.push_back(2);
    moveSeq.push_back(3);
    moveSeq.push_back(4);
    moveSeq.push_back(5);
    moveSeq.push_back(6);
    moveSeq.push_back(7);

#if !defined(USE_SIMPLE_MOVE_SEQ) || (USE_SIMPLE_MOVE_SEQ == 0)
    printf("moveSeq.size() = %u\n", (std::uint32_t)moveSeq.size());
    printf("moveSeq.inner_seq() = 0x%p\n", (std::size_t *)moveSeq.inner_seq());
    printf("\n");
#endif

    moveSeq.push_back(0);
    moveSeq.push_back(1);
    moveSeq.push_back(2);
    moveSeq.push_back(3);
    moveSeq.push_back(4);
    moveSeq.push_back(5);
    moveSeq.push_back(6);
    moveSeq.push_back(7);

    moveSeq.push_back(0);
    moveSeq.push_back(1);
    moveSeq.push_back(2);
    moveSeq.push_back(3);
    moveSeq.push_back(4);
    moveSeq.push_back(5);
    moveSeq.push_back(6);
    moveSeq.push_back(7);

    moveSeq.push_back(0);
    moveSeq.push_back(1);
    moveSeq.push_back(2);
    moveSeq.push_back(3);
    moveSeq.push_back(4);
    moveSeq.push_back(5);
    moveSeq.push_back(6);
    moveSeq.push_back(7);

    moveSeq.push_back(0);
    moveSeq.push_back(1);
    moveSeq.push_back(2);
    moveSeq.push_back(3);
    moveSeq.push_back(4);
    moveSeq.push_back(5);
    moveSeq.push_back(6);
    moveSeq.push_back(7);

#if !defined(USE_SIMPLE_MOVE_SEQ) || (USE_SIMPLE_MOVE_SEQ == 0)
    printf("moveSeq.size() = %u\n", (std::uint32_t)moveSeq.size());
    printf("moveSeq.inner_seq() = 0x%p\n", (std::size_t *)moveSeq.inner_seq());
    for (std::size_t i = 0; i < moveSeq.unit_capacity(); i++) {
        printf("moveSeq.unit[%u] = 0x%p\n", (std::uint32_t)i, (std::size_t *)moveSeq.units(i));
    }
    printf("\n");
#endif
}

void find_uint16_test()
{
    std::uint16_t indexs[128];
    std::fill_n(indexs, 128, 0x1235);
    indexs[100] = 0x1234;
    int index = Algorithm::find_uint16_sse2(indexs, 128, 0x1234);
};

void jm_mallc_SizeClass_test()
{
    ThreadMalloc<0>::SizeClass sizeClass;

    std::size_t index, index1, index2;
    index = sizeClass.sizeToIndex(0);
    assert(index == 0);

    index  = sizeClass.sizeToIndex(4095);
    index1 = sizeClass.sizeToIndex(4096);
    index2 = sizeClass.sizeToIndex(4097);
    assert(index == 92);
    assert(index1 == 92);
    assert(index2 == (index1 + 1));

    index  = sizeClass.sizeToIndex(16383);
    index1 = sizeClass.sizeToIndex(16384);
    index2 = sizeClass.sizeToIndex(16385);
    assert(index == (92 + 96));
    assert(index1 == (92 + 96));
    assert(index2 == (index1 + 1));

    (void)index;
    (void)index1;
    (void)index2;
}

void jm_mallc_ThreadMalloc_test()
{
    ThreadMalloc<0> threadMalloc;
    threadMalloc.getInstance();

    threadMalloc.shutdown();
}

void jm_mallc_test()
{
    jm_mallc_SizeClass_test();
    jm_mallc_ThreadMalloc_test();
}

void UnitTest()
{
    SparseTrieBitset_test();
    //MoveSeq_test();
    find_uint16_test();
    jm_mallc_test();
}
