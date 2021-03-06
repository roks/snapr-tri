#ifndef CONV_H
#define CONV_H

namespace TSnap {

template<class PGraph>
PGraph ToGraph(PTable Table, const TStr& SrcCol, const TStr& DstCol, TAttrAggr AggrPolicy)
{
	PGraph Graph = PGraph::TObj::New();

  const TAttrType NodeType = Table->GetColType(SrcCol);
  Assert(NodeType == Table->GetColType(DstCol));
  const TInt SrcColIdx = Table->GetColIdx(SrcCol);
  const TInt DstColIdx = Table->GetColIdx(DstCol);
  
  // make single pass over all rows in the table
  if (NodeType == atInt) {
    for (int CurrRowIdx = 0; CurrRowIdx < (Table->Next).Len(); CurrRowIdx++) {
      if ((Table->Next)[CurrRowIdx] == Table->Invalid) { continue; }
      // add src and dst nodes to graph if they are not seen earlier
      TInt SVal = (Table->IntCols)[SrcColIdx][CurrRowIdx];
      TInt DVal = (Table->IntCols)[DstColIdx][CurrRowIdx];
      Graph->AddNodeUnchecked(SVal);
      Graph->AddNodeUnchecked(DVal);
      Graph->AddEdgeUnchecked(SVal, DVal);
    }
  } else if (NodeType == atFlt) {
    // node values - i.e. the unique values of src/dst col
    //THashSet<TInt> IntNodeVals; // for both int and string node attr types.
    THash<TFlt, TInt> FltNodeVals;
    for (int CurrRowIdx = 0; CurrRowIdx < (Table->Next).Len(); CurrRowIdx++) {
      if ((Table->Next)[CurrRowIdx] == Table->Invalid) { continue; }
      // add src and dst nodes to graph if they are not seen earlier
      TInt SVal, DVal;
      TFlt FSVal = (Table->FltCols)[SrcColIdx][CurrRowIdx];
      SVal = Table->CheckAndAddFltNode(Graph, FltNodeVals, FSVal);
      TFlt FDVal = (Table->FltCols)[SrcColIdx][CurrRowIdx];
      DVal = Table->CheckAndAddFltNode(Graph, FltNodeVals, FDVal);
      Graph->AddEdge(SVal, DVal);
    }  
  } else {
    for (int CurrRowIdx = 0; CurrRowIdx < (Table->Next).Len(); CurrRowIdx++) {
      if ((Table->Next)[CurrRowIdx] == Table->Invalid) { continue; }
      // add src and dst nodes to graph if they are not seen earlier
      TInt SVal = (Table->StrColMaps)[SrcColIdx][CurrRowIdx];
      if (strlen(Table->GetContextKey(SVal)) == 0) { continue; }  //illegal value
      TInt DVal = (Table->StrColMaps)[DstColIdx][CurrRowIdx];
      if (strlen(Table->GetContextKey(DVal)) == 0) { continue; }  //illegal value
      Graph->AddNode(SVal);
      Graph->AddNode(DVal);
      Graph->AddEdge(SVal, DVal);
    }  
  }
  
  Graph->SortNodeAdjV();
  return Graph;
}

template<class PGraph> 
PGraph ToNetwork(PTable Table, const TStr& SrcCol, const TStr& DstCol, 
	TStrV& SrcAttrV, TStrV& DstAttrV, TStrV& EdgeAttrV, TAttrAggr AggrPolicy)
{
	PGraph Graph = PGraph::TObj::New();

  const TAttrType NodeType = Table->GetColType(SrcCol);
  Assert(NodeType == Table->GetColType(DstCol));
  const TInt SrcColIdx = Table->GetColIdx(SrcCol);
  const TInt DstColIdx = Table->GetColIdx(DstCol);

	//Table->AddGraphAttributeV(SrcAttrV, false, true, false);
	//Table->AddGraphAttributeV(DstAttrV, false, false, true);
	//Table->AddGraphAttributeV(EdgeAttrV, true, false, true);

  // node values - i.e. the unique values of src/dst col
  //THashSet<TInt> IntNodeVals; // for both int and string node attr types.
  THash<TFlt, TInt> FltNodeVals;

  // node attributes
  THash<TInt, TStrIntVH> NodeIntAttrs;
  THash<TInt, TStrFltVH> NodeFltAttrs;
  THash<TInt, TStrStrVH> NodeStrAttrs;

  // make single pass over all rows in the table
  for (int CurrRowIdx = 0; CurrRowIdx < (Table->Next).Len(); CurrRowIdx++) {
    if ((Table->Next)[CurrRowIdx] == Table->Invalid) { continue; }

    // add src and dst nodes to graph if they are not seen earlier
   TInt SVal, DVal;
    if (NodeType == atFlt) {
      TFlt FSVal = (Table->FltCols)[SrcColIdx][CurrRowIdx];
      SVal = Table->CheckAndAddFltNode(Graph, FltNodeVals, FSVal);
      TFlt FDVal = (Table->FltCols)[SrcColIdx][CurrRowIdx];
      DVal = Table->CheckAndAddFltNode(Graph, FltNodeVals, FDVal);
    } else if (NodeType == atInt || NodeType == atStr) {
      if (NodeType == atInt) {
        SVal = (Table->IntCols)[SrcColIdx][CurrRowIdx];
        DVal = (Table->IntCols)[DstColIdx][CurrRowIdx];
      } else {
        SVal = (Table->StrColMaps)[SrcColIdx][CurrRowIdx];
        if (strlen(Table->GetContextKey(SVal)) == 0) { continue; }  //illegal value
        DVal = (Table->StrColMaps)[DstColIdx][CurrRowIdx];
        if (strlen(Table->GetContextKey(DVal)) == 0) { continue; }  //illegal value
      }
      if (!Graph->IsNode(SVal)) {Graph->AddNode(SVal); }
      if (!Graph->IsNode(DVal)) {Graph->AddNode(DVal); }
      //CheckAndAddIntNode(Graph, IntNodeVals, SVal);
      //CheckAndAddIntNode(Graph, IntNodeVals, DVal);
    } 

    // add edge and edge attributes 
    Graph->AddEdge(SVal, DVal, CurrRowIdx);

		// Aggregate edge attributes and add to graph
		for (TInt i = 0; i < Table->EdgeAttrV.Len(); i++) {
			TStr ColName = EdgeAttrV[i];
			TAttrType T = Table->GetColType(ColName);
			TInt Index = Table->GetColIdx(ColName);
			switch (T) {
				case atInt:
					Graph->AddIntAttrDatE(CurrRowIdx, Table->IntCols[Index][CurrRowIdx], ColName);
					break;
				case atFlt:
					Graph->AddFltAttrDatE(CurrRowIdx, Table->FltCols[Index][CurrRowIdx], ColName);
					break;
				case atStr:
					Graph->AddStrAttrDatE(CurrRowIdx, Table->GetStrVal(Index, CurrRowIdx), ColName);
					break;
			}
		}

    // get src and dst node attributes into hashmaps
    if ((Table->SrcNodeAttrV).Len() > 0) { 
      Table->AddNodeAttributes(SVal, Table->SrcNodeAttrV, CurrRowIdx, NodeIntAttrs, NodeFltAttrs, NodeStrAttrs);
  	}

    if ((Table->DstNodeAttrV).Len() > 0) {
      Table->AddNodeAttributes(DVal, Table->DstNodeAttrV, CurrRowIdx, NodeIntAttrs, NodeFltAttrs, NodeStrAttrs);
    }
  }

  // aggregate node attributes and add to graph
  if ((Table->SrcNodeAttrV).Len() > 0 || (Table->DstNodeAttrV).Len() > 0) {
    for (TNEANet::TNodeI NodeI = Graph->BegNI(); NodeI < Graph->EndNI(); NodeI++) {
      TInt NId = NodeI.GetId();
      if (NodeIntAttrs.IsKey(NId)) {
        TStrIntVH IntAttrVals = NodeIntAttrs.GetDat(NId);
        for (TStrIntVH::TIter it = IntAttrVals.BegI(); it < IntAttrVals.EndI(); it++) {
          TInt AttrVal = Table->AggregateVector<TInt>(it.GetDat(), AggrPolicy);
          Graph->AddIntAttrDatN(NId, AttrVal, it.GetKey());
        }
      }
      if (NodeFltAttrs.IsKey(NId)) {
        TStrFltVH FltAttrVals = NodeFltAttrs.GetDat(NId);
        for (TStrFltVH::TIter it = FltAttrVals.BegI(); it < FltAttrVals.EndI(); it++) {
          TFlt AttrVal = Table->AggregateVector<TFlt>(it.GetDat(), AggrPolicy);
          Graph->AddFltAttrDatN(NId, AttrVal, it.GetKey());
        }
      }
      if (NodeStrAttrs.IsKey(NId)) {
        TStrStrVH StrAttrVals = NodeStrAttrs.GetDat(NId);
        for (TStrStrVH::TIter it = StrAttrVals.BegI(); it < StrAttrVals.EndI(); it++) {
          TStr AttrVal = Table->AggregateVector<TStr>(it.GetDat(), AggrPolicy);
          Graph->AddStrAttrDatN(NId, AttrVal, it.GetKey());
        }
      }
    }
  }

  return Graph;
}

template<class PGraph>
PGraph ToNetwork(PTable Table, const TStr& SrcCol, const TStr& DstCol, TAttrAggr AggrPolicy)
{
	TStrV V;
	return ToNetwork<PNEANet>(Table, SrcCol, DstCol, V, V, V, AggrPolicy);
}

template<class PGraphMP>
PGraphMP ToGraphMP(PTable Table, const TStr& SrcCol, const TStr& DstCol) {
  int MaxThreads = omp_get_max_threads();
  double start = omp_get_wtime();
  //PGraphMP Graph = TNGraphMP::New(4850000, 69000000);
  PGraphMP Graph = TNGraphMP::New(41700000, 1470000000);
  double endReserve = omp_get_wtime();
  printf("Reserve time = %f\n", endReserve-start);

  //printf("TNGraphMP::New nodes %d, edges %d\n", Graph->GetNodes(), Graph->GetEdges());
  printf("TNGraphMP::New reserved %d\n", Graph->Reserved());
  
  // make single pass over all rows in the table
  
  int Length = Graph->Reserved();
  int Threads = MaxThreads/2; //40;
  int Delta = (Length + Threads - 1) / Threads;

  printf("Build Threads %d\n", Threads);

  TIntV OutVec(Length);
  TIntV InVec(Length);

  const TInt SrcColIdx = Table->GetColIdx(SrcCol);
  const TInt DstColIdx = Table->GetColIdx(DstCol);

  printf("%d %d\n", SrcColIdx.Val, DstColIdx.Val);
  const int L1 = Length;

  //int *ps = &IntCols[SrcColIdx][0].Val;
  //int *pd = &IntCols[DstColIdx][0].Val;

  //
  // build the node hash table, count the size of edge lists
  //
  const int Last = Table->Next.Len();
  int Nodes = 0;
  omp_set_num_threads(Threads);
  #pragma omp parallel for schedule(static, Delta) reduction(+:Nodes)
  for (int CurrRowIdx = 0; CurrRowIdx < Last; CurrRowIdx++) {
    //if (Next[CurrRowIdx] == Invalid) {continue;}

    TInt SVal = Table->IntCols[SrcColIdx][CurrRowIdx];
    TInt DVal = Table->IntCols[DstColIdx][CurrRowIdx];

    //SVal = *ps++;
    //DVal = *pd++;

    int SrcIdx = abs((SVal.GetPrimHashCd()) % L1);
    //int SrcIdx = abs((17*SVal.Val) % L1);
    if (!Graph->AddOutEdge1(SrcIdx, SVal, DVal)) {
      Nodes++;
    }
    __sync_fetch_and_add(&OutVec[SrcIdx].Val, 1);
    //OutVec[SrcIdx]++;

    int DstIdx = abs((DVal.GetPrimHashCd()) % L1);
    //int DstIdx = abs((17*DVal.Val) % L1);
    if (!Graph->AddInEdge1(DstIdx, SVal, DVal)) {
      Nodes++;
    }
    __sync_fetch_and_add(&InVec[DstIdx].Val, 1);
    //InVec[DstIdx]++;
  }
  double endNodes = omp_get_wtime();
  printf("Add Node time = %f\n", endNodes-endReserve);
  Graph->SetNodes(Nodes);
  printf("nodes %d\n", Nodes);

  uint Edges = 0;
  for (int i = 0; i < Length; i++) {
    Edges += OutVec[i] + InVec[i];
  }
  printf("edges %d\n", (int) (Edges/2));

  //
  // allocate edge lists
  //
  
  Length = Graph->Reserved();
  Threads = 1;
  Delta = (Length + Threads - 1) / Threads;

  printf("Allocate Threads %d\n", Threads);

  // requires memory allocation, no threads seems the best
  // #pragma omp parallel for schedule(static,Delta)
  // omp_set_num_threads(Threads);
  // #pragma omp parallel for schedule(static)
  for (int Idx = 0; Idx < Length; Idx++) {
    if (OutVec[Idx] > 0 || InVec[Idx] > 0) {
      Graph->ReserveNodeDegs(Idx, InVec[Idx], OutVec[Idx]);
    }
  }

  double endAlloc = omp_get_wtime();
  printf("Alloc time = %f\n", endAlloc-endNodes);

  printf("allocated edge lists\n");

  //
  // assign edges
  //

  Length = Graph->Reserved();
  Threads = MaxThreads;//80;
  Delta = (Length + Threads - 1) / Threads;

  printf("Assign Threads %d\n", Threads);
  omp_set_num_threads(Threads);
  #pragma omp parallel for schedule(static,Delta)
  for (int CurrRowIdx = 0; CurrRowIdx < Last; CurrRowIdx++) {
    //if (Next[CurrRowIdx] == Invalid) {continue;}

    TInt SVal = Table->IntCols[SrcColIdx][CurrRowIdx];
    TInt DVal = Table->IntCols[DstColIdx][CurrRowIdx];

    Graph->AddOutEdge2(SVal, DVal);
    Graph->AddInEdge2(SVal, DVal);
  }

  double endPopulate = omp_get_wtime();
  printf("Populate time = %f\n", endPopulate-endAlloc);

  printf("populated edge lists\n");

  //
  // sort edges
  //

  Length = Graph->Reserved();
  Threads = MaxThreads*2;//160;
  Delta = (Length + Threads - 1) / Threads;
  printf("Sort Threads %d\n", Threads);

  omp_set_num_threads(Threads);
  //#pragma omp parallel for schedule(static,Delta)
  #pragma omp parallel for schedule(dynamic)
  for (int Idx = 0; Idx < Length; Idx++) {
    if (OutVec[Idx] > 0 || InVec[Idx] > 0) {
      Graph->SortEdges(Idx, InVec[Idx], OutVec[Idx]);
    }
  }

  double endSort = omp_get_wtime();
  printf("Sort time = %f\n", endSort-endPopulate);

  printf("sorted edge lists\n");

  return Graph;
}

template<class PGraphMP>
PGraphMP ToGraphMP2(PTable Table, const TStr& SrcCol, const TStr& DstCol) {
  double start = omp_get_wtime();
  const TInt SrcColIdx = Table->GetColIdx(SrcCol);
  const TInt DstColIdx = Table->GetColIdx(DstCol);
  const TInt NumRows = Table->NumValidRows;

  TIntV SrcCol1, DstCol1, SrcCol2, DstCol2;
  //TIntPrV SrcPr, DstPr;

  #pragma omp parallel sections num_threads(4)
  {
    #pragma omp section 
    { SrcCol1.Reserve(NumRows, NumRows); } 
    #pragma omp section 
    { SrcCol2.Reserve(NumRows, NumRows); } 
    #pragma omp section 
    { DstCol1.Reserve(NumRows, NumRows); }
    #pragma omp section 
    { DstCol2.Reserve(NumRows, NumRows); }
    // #pragma omp section 
    // { SrcPr.Reserve(NumRows, NumRows); }
    // #pragma omp section 
    // { DstPr.Reserve(NumRows, NumRows); }
  }

  double endResize = omp_get_wtime();
  printf("Resize time = %f\n", endResize-start);

  TIntPrV Partitions;
  Table->GetPartitionRanges(Partitions, omp_get_max_threads());
  TInt PartitionSize = Partitions[0].GetVal2()-Partitions[0].GetVal1()+1;

  double endPartition = omp_get_wtime();
  printf("Partition time = %f\n", endPartition-endResize);

  omp_set_num_threads(omp_get_max_threads());
  #pragma omp parallel for schedule(static) 
  for (int i = 0; i < Partitions.Len(); i++) {
    TRowIterator RowI(Partitions[i].GetVal1(), Table());
    TRowIterator EndI(Partitions[i].GetVal2(), Table());
    while (RowI < EndI) {
      TInt RowId = RowI.GetRowIdx();
      SrcCol1[RowId] = RowI.GetIntAttr(SrcColIdx);
      SrcCol2[RowId] = RowI.GetIntAttr(SrcColIdx);
      DstCol1[RowId] = RowI.GetIntAttr(DstColIdx);
      DstCol2[RowId] = RowI.GetIntAttr(DstColIdx);
      RowI++;
    }
  }

  // omp_set_num_threads(omp_get_max_threads());
  // #pragma omp parallel for schedule(static) 
  // for (int i = 0; i < Partitions.Len(); i++) {
  //   TRowIterator RowI(Partitions[i].GetVal1(), Table());
  //   TRowIterator EndI(Partitions[i].GetVal2(), Table());
  //   while (RowI < EndI) {
  //     TInt RowId = RowI.GetRowIdx();
  //     SrcPr[RowId] = TIntPr(RowI.GetIntAttr(SrcColIdx), RowI.GetIntAttr(DstColIdx));
  //     DstPr[RowId] = TIntPr(RowI.GetIntAttr(DstColIdx), RowI.GetIntAttr(SrcColIdx));
  //     RowI++;
  //   }
  // }  

  double endCopy = omp_get_wtime();
  printf("Copy time = %f\n", endCopy-endPartition);

  // SrcPr.Sort();
  // DstPr.Sort();
  // double endPrSort = omp_get_wtime();
  // printf("Pair sort time = %f\n", endPrSort-endCopy);

  // omp_set_num_threads(omp_get_max_threads());
  // #pragma omp parallel for schedule(static) 
  // for (int i = 0; i < Partitions.Len(); i++) {
  //   TRowIterator RowI(Partitions[i].GetVal1(), Table());
  //   TRowIterator EndI(Partitions[i].GetVal2(), Table());
  //   while (RowI < EndI) {
  //     TInt RowId = RowI.GetRowIdx();
  //     SrcCol1[RowId] = SrcPr[RowId].Val1;
  //     SrcCol2[RowId] = DstPr[RowId].Val2;
  //     DstCol1[RowId] = SrcPr[RowId].Val2;
  //     DstCol2[RowId] = DstPr[RowId].Val1;
  //     RowI++;
  //   }
  // }

  omp_set_num_threads(omp_get_max_threads());
  #pragma omp parallel 
  {
    #pragma omp single nowait
    {
      #pragma omp task untied shared(SrcCol1, DstCol1)
      { TTable::QSortKeyVal(SrcCol1, DstCol1, 0, NumRows-1); }
    }
    #pragma omp single nowait
    {
      #pragma omp task untied shared(SrcCol2, DstCol2)
     { TTable::QSortKeyVal(DstCol2, SrcCol2, 0, NumRows-1); }
    }
    #pragma omp taskwait
  }

  // TTable::PSRSKeyVal(SrcCol1, DstCol1, 0, NumRows-1);
  // TTable::PSRSKeyVal(DstCol2, SrcCol2, 0, NumRows-1);

  // TInt IsS = TTable::CheckSortedKeyVal(SrcCol1, DstCol1, 0, NumRows-1);
  // TInt IsD = TTable::CheckSortedKeyVal(DstCol2, SrcCol2, 0, NumRows-1);
  // printf("IsSorted = %d %d\n", IsS.Val, IsD.Val);

  double endSort = omp_get_wtime();
  printf("Sort time = %f\n", endSort-endCopy);  
  //return TNGraphMP::New(10, 100);

  TInt NumThreads = omp_get_max_threads();
  TInt PartSize = (NumRows/NumThreads);

  TIntV SrcOffsets, DstOffsets;
  SrcOffsets.Add(0);
  for (TInt i = 1; i < NumThreads; i++) {
    TInt CurrOffset = i * PartSize;
    // TODO: ensure that CurrOffset stays within the current thread's partition
    while (CurrOffset < (i+1) * PartSize && 
          SrcCol1[CurrOffset-1] == SrcCol1[CurrOffset]) { 
      CurrOffset++; 
    }
    Assert(CurrOffset < (i+1) * PartSize);
    SrcOffsets.Add(CurrOffset);
  }
  SrcOffsets.Add(NumRows);

  DstOffsets.Add(0);
  for (TInt i = 1; i < NumThreads; i++) {
    TInt CurrOffset = i * PartSize;
    // TODO: ensure that CurrOffset stays within the current thread's partition
    while (CurrOffset < (i+1) * PartSize && 
          DstCol2[CurrOffset-1] == DstCol2[CurrOffset]) { 
      CurrOffset++; 
    }
    Assert(CurrOffset < (i+1) * PartSize);
    DstOffsets.Add(CurrOffset);
  }
  DstOffsets.Add(NumRows);

  TIntV SrcNodeCounts, DstNodeCounts;
  SrcNodeCounts.Reserve(NumThreads, NumThreads);
  DstNodeCounts.Reserve(NumThreads, NumThreads);

  #pragma omp parallel for schedule(dynamic)
  for (int t = 0; t < 2*NumThreads; t++) {
    TInt i = t/2;
    if (t % 2 == 0 && SrcOffsets[i] != SrcOffsets[i+1]) { 
      SrcNodeCounts[i] = 1;
      TInt CurrNode = SrcCol1[SrcOffsets[i]];
      for (TInt j = SrcOffsets[i]+1; j < SrcOffsets[i+1]; j++) {
        while (j < SrcOffsets[i+1] && SrcCol1[j] == CurrNode) { j++; }
        if (j < SrcOffsets[i+1]) {
          SrcNodeCounts[i]++;
          CurrNode = SrcCol1[j];
        }
      }
    }
    if (t % 2 == 1 && DstOffsets[i] != DstOffsets[i+1]) { 
      DstNodeCounts[i] = 1;
      TInt CurrNode = DstCol2[DstOffsets[i]];
      for (TInt j = DstOffsets[i]+1; j < DstOffsets[i+1]; j++) {
        while (j < DstOffsets[i+1] && DstCol2[j] == CurrNode) { j++; }
        if (j < DstOffsets[i+1]) {
          DstNodeCounts[i]++;
          CurrNode = DstCol2[j];
        }
      }
    }
  }

  for (TInt i = 0; i < SrcNodeCounts.Len(); i++) {
    printf("%d ", SrcNodeCounts[i].Val);
  }
  printf("\n");
  for (TInt i = 0; i < DstNodeCounts.Len(); i++) {
    printf("%d ", DstNodeCounts[i].Val);
  }
  printf("\n");

  TInt TotalSrcNodes = 0;
  TIntV SrcIdOffsets;
  for (int i = 0; i < NumThreads; i++) {
    SrcIdOffsets.Add(TotalSrcNodes);
    TotalSrcNodes += SrcNodeCounts[i];
  }

  TInt TotalDstNodes = 0;
  TIntV DstIdOffsets;
  for (int i = 0; i < NumThreads; i++) {
    DstIdOffsets.Add(TotalDstNodes);
    TotalDstNodes += DstNodeCounts[i];
  }

  printf("Total Src = %d, Total Dst = %d\n", TotalSrcNodes.Val, TotalDstNodes.Val);

  TIntPrV SrcNodeIds, DstNodeIds;
  #pragma omp parallel sections
  {
    #pragma omp section
    { SrcNodeIds.Reserve(TotalSrcNodes, TotalSrcNodes); }
    #pragma omp section
    { DstNodeIds.Reserve(TotalDstNodes, TotalDstNodes); }
  }

  #pragma omp parallel for schedule(dynamic)
  for (int t = 0; t < 2*NumThreads; t++) {
    TInt i = t/2;
    if (t % 2 == 0 && SrcOffsets[i] != SrcOffsets[i+1]) {
      TInt CurrNode = SrcCol1[SrcOffsets[i]];
      TInt ThreadOffset = SrcIdOffsets[i];
      SrcNodeIds[ThreadOffset] = TIntPr(CurrNode, SrcOffsets[i]);
      TInt CurrCount = 1;
      for (TInt j = SrcOffsets[i]+1; j < SrcOffsets[i+1]; j++) {
        while (j < SrcOffsets[i+1] && SrcCol1[j] == CurrNode) { j++; }
        if (j < SrcOffsets[i+1]) {
          CurrNode = SrcCol1[j];
          SrcNodeIds[ThreadOffset+CurrCount] = TIntPr(CurrNode, j);
          CurrCount++;
        }
      }
    }
    if (t % 2 == 1 && DstOffsets[i] != DstOffsets[i+1]) {
      TInt CurrNode = DstCol2[DstOffsets[i]];
      TInt ThreadOffset = DstIdOffsets[i];
      DstNodeIds[ThreadOffset] = TIntPr(CurrNode, DstOffsets[i]);
      TInt CurrCount = 1;
      for (TInt j = DstOffsets[i]+1; j < DstOffsets[i+1]; j++) {
        while (j < DstOffsets[i+1] && DstCol2[j] == CurrNode) { j++; }
        if (j < DstOffsets[i+1]) {
          CurrNode = DstCol2[j];
          DstNodeIds[ThreadOffset+CurrCount] = TIntPr(CurrNode, j);
          CurrCount++;
        }
      }
    }
  }

  double endNode = omp_get_wtime();
  printf("Node time = %f\n", endNode-endSort);

  TIntTrV Nodes;
  Nodes.Reserve(TotalSrcNodes+TotalDstNodes);

  double endNodeResize = omp_get_wtime();
  printf("(NodeResize time = %f)\n", endNodeResize-endNode);

  TInt i = 0, j = 0;
  while (i < TotalSrcNodes && j < TotalDstNodes) {
    if (SrcNodeIds[i].Val1 == DstNodeIds[j].Val1) {
      Nodes.Add(TIntTr(SrcNodeIds[i].Val1, i, j));
      i++;
      j++;
    } else if (SrcNodeIds[i].Val1 < DstNodeIds[j].Val1) {
      Nodes.Add(TIntTr(SrcNodeIds[i].Val1, i, -1));
      i++;
    } else {
      Nodes.Add(TIntTr(DstNodeIds[j].Val1, -1, j));
      j++;
    }
  }
  for (; i < TotalSrcNodes; i++) { Nodes.Add(TIntTr(SrcNodeIds[i].Val1, i, -1)); }
  for (; j < TotalDstNodes; j++) { Nodes.Add(TIntTr(DstNodeIds[j].Val1, -1, j)); }

  double endMerge = omp_get_wtime();
  printf("Merge time = %f\n", endMerge-endNode);  

  TInt NumNodes = Nodes.Len();
  printf("NumNodes = %d\n", NumNodes.Val);

  PGraphMP Graph = TNGraphMP::New(NumNodes, NumRows);
  NumThreads = 1;
  int Delta = (NumNodes+NumThreads-1)/NumThreads;

  TVec<TIntV> InVV(NumNodes);
  TVec<TIntV> OutVV(NumNodes);

  omp_set_num_threads(NumThreads);
  #pragma omp parallel for schedule(static,Delta)
  for (int m = 0; m < NumNodes; m++) {
    //double startTr = omp_get_wtime();
    //TIntV OutV, InV;
    TInt n, i, j;
    Nodes[m].GetVal(n, i, j);
    if (i >= 0) { 
      TInt Offset = SrcNodeIds[i].GetVal2();
      TInt Sz = DstCol1.Len()-Offset;
      if (i < SrcNodeIds.Len()-1) { Sz = SrcNodeIds[i+1].GetVal2()-Offset; }
      //printf("OutV: %d %d %d\n", n.Val, Offset.Val, Sz.Val);
      OutVV[m].Reserve(Sz); 
    }
    if (j >= 0) {
      TInt Offset = DstNodeIds[j].GetVal2();
      TInt Sz = SrcCol2.Len()-Offset;
      if (j < DstNodeIds.Len()-1) { Sz = DstNodeIds[j+1].GetVal2()-Offset; }
      //printf("OutV: %d %d %d\n", n.Val, Offset.Val, Sz.Val);
      InVV[m].Reserve(Sz);
    }
    //double endTr = omp_get_wtime();
    //printf("Thread=%d, i=%d, t=%f\n", omp_get_thread_num(), m, endTr-startTr);
  }

  double endAlloc = omp_get_wtime();
  printf("Alloc time = %f\n", endAlloc-endMerge);  

  NumThreads = omp_get_max_threads();
  Delta = (NumNodes+NumThreads-1)/(10*NumThreads);
  omp_set_num_threads(NumThreads);
  #pragma omp parallel for schedule(dynamic)
  for (int m = 0; m < NumNodes; m++) {
    //double startTr = omp_get_wtime();
    //TIntV OutV, InV;
    TInt n, i, j;
    Nodes[m].GetVal(n, i, j);
    if (i >= 0) { 
      TInt Offset = SrcNodeIds[i].GetVal2();
      TInt Sz = DstCol1.Len()-Offset;
      if (i < SrcNodeIds.Len()-1) { Sz = SrcNodeIds[i+1].GetVal2()-Offset; }
      //printf("OutV: %d %d %d\n", n.Val, Offset.Val, Sz.Val);
      OutVV[m].CopyUniqueFrom(DstCol1, Offset, Sz); 
    }
    if (j >= 0) {
      TInt Offset = DstNodeIds[j].GetVal2();
      TInt Sz = SrcCol2.Len()-Offset;
      if (j < DstNodeIds.Len()-1) { Sz = DstNodeIds[j+1].GetVal2()-Offset; }
      //printf("OutV: %d %d %d\n", n.Val, Offset.Val, Sz.Val);
      InVV[m].CopyUniqueFrom(SrcCol2, Offset, Sz);
    }
    Graph->AddNodeWithEdges(n, InVV[m], OutVV[m]);
    //double endTr = omp_get_wtime();
    //printf("Thread=%d, i=%d, t=%f\n", omp_get_thread_num(), m, endTr-startTr);
  }
  Graph->SetNodes(NumNodes);

  double endAdd = omp_get_wtime();
  printf("Add time = %f\n", endAdd-endAlloc);  

  return Graph;
}
};

#endif //CONV_H
