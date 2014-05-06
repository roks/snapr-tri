namespace TSnap {

// Rok #13
//template<class PGraph> 
int64 CountTriangles1(const TVec<TNGraph::TNodeI,int>& NV, const TIntV& IndV, const TIntV& MapV) {
  struct timeval start, end;
  float delta;
  TTmProfiler Profiler;
  int TimerId = Profiler.AddTimer("Profiler");

  int ind = MapV.Len();

  Profiler.ResetTimer(TimerId);
  Profiler.StartTimer(TimerId);
  gettimeofday(&start, NULL);

  TVec<TIntV> HigherDegNbrV(ind);

  for (int i = 0; i < ind; i++) {
    HigherDegNbrV[i] = TVec<TInt>();
    HigherDegNbrV[i].Reserve(NV[i].GetDeg());
    HigherDegNbrV[i].Reduce(0);
  }

  gettimeofday(&end, NULL);
  Profiler.StopTimer(TimerId);
  delta = ((end.tv_sec  - start.tv_sec) * 1000000u +
            end.tv_usec - start.tv_usec) / 1.e6;
  printf("__valloc__\ttime %7.3f\tcpu %8.3f\n", delta, Profiler.GetTimerSec(TimerId));

  Profiler.ResetTimer(TimerId);
  Profiler.StartTimer(TimerId);
  gettimeofday(&start, NULL);

#pragma omp parallel for schedule(dynamic)
  for (TInt i = 0; i < ind; i++) {
    TNGraph::TNodeI NI = NV[i];
    //HigherDegNbrV[i] = TVec<TInt>();
    //HigherDegNbrV[i].Reserve(NI.GetDeg());
    //HigherDegNbrV[i].Reduce(0);

    GetMergeSortedV(HigherDegNbrV[i], NI);

    int k = 0;
    for (TInt j = 0; j < HigherDegNbrV[i].Len(); j++) {
      TInt Vert = HigherDegNbrV[i][j];
      TInt Deg = NV[IndV[Vert]].GetDeg();
      if (Deg > NI.GetDeg() ||
         (Deg == NI.GetDeg() && Vert > NI.GetId())) {
        HigherDegNbrV[i][k] = Vert;
        k++;
      }
    }
    HigherDegNbrV[i].Reduce(k);
  }

  gettimeofday(&end, NULL);
  Profiler.StopTimer(TimerId);
  delta = ((end.tv_sec  - start.tv_sec) * 1000000u +
            end.tv_usec - start.tv_usec) / 1.e6;
  printf("__sort__\ttime %7.3f\tcpu %8.3f\n", delta, Profiler.GetTimerSec(TimerId));

  Profiler.ResetTimer(TimerId);
  Profiler.StartTimer(TimerId);
  gettimeofday(&start, NULL);

  int64 cnt = 0;
#pragma omp parallel for schedule(dynamic) reduction(+:cnt)
  for (TInt i = 0; i < HigherDegNbrV.Len(); i++) {
    for (TInt j = 0; j < HigherDegNbrV[i].Len(); j++) {
      //TInt NbrInd = H.GetDat(HigherDegNbrV[i][j]);
      TInt NbrInd = IndV[HigherDegNbrV[i][j]];

      int64 num = GetCommon(HigherDegNbrV[i], HigherDegNbrV[NbrInd]);
      cnt += num;
    }
  }

  gettimeofday(&end, NULL);
  Profiler.StopTimer(TimerId);
  delta = ((end.tv_sec  - start.tv_sec) * 1000000u +
            end.tv_usec - start.tv_usec) / 1.e6;
  printf("__count__\ttime %7.3f\tcpu %8.3f\n", delta, Profiler.GetTimerSec(TimerId));

  return cnt;
}

#if 0
// Arijit
void GetMergeSortedV(TIntV& NeighbourV, TNGraph::TNodeI NI) {
  int ind, j, k;
  ind = j = k = 0;
  while (j < NI.GetInDeg() && k < NI.GetOutDeg()) {
    int v1 = NI.GetInNId(j);
    int v2 = NI.GetOutNId(k);
    if (v1 <= v2) {
      if ((ind == 0) || (NeighbourV[ind-1] != v1)) {
        NeighbourV.Add(v1);
        ind += 1;
      }
      j += 1;
    }
    else {
      if ((ind == 0) || (NeighbourV[ind-1] != v2)) {
        NeighbourV.Add(v2);
        ind += 1;
      }
      k += 1;
    }
  }
  while (j < NI.GetInDeg()) {
    int v = NI.GetInNId(j);
    if ((ind == 0) || (NeighbourV[ind-1] != v)) {
        NeighbourV.Add(v);
        ind += 1;
    }
    j += 1;
  }
  while (k < NI.GetOutDeg()) {
    int v = NI.GetOutNId(k);
    if ((ind == 0) || (NeighbourV[ind-1] != v)) {
        NeighbourV.Add(v);
        ind += 1;
    }
    k += 1;
  }
}
#endif

#if 0
// Rok #5
void GetMergeSortedV(TIntV& NeighbourV, TNGraph::TNodeI NI) {
  int j = 0;
  int k = 0;
  int prev = -1;
  while (j < NI.GetInDeg() && k < NI.GetOutDeg()) {
    int v1 = NI.GetInNId(j);
    int v2 = NI.GetOutNId(k);
    if (v1 <= v2) {
      if (prev != v1) {
        NeighbourV.Add(v1);
        prev = v1;
      }
      j += 1;
    } else {
      if (prev != v2) {
        NeighbourV.Add(v2);
        prev = v2;
      }
      k += 1;
    }
  }
  while (j < NI.GetInDeg()) {
    int v = NI.GetInNId(j);
    if (prev != v) {
      NeighbourV.Add(v);
      prev = v;
    }
    j += 1;
  }
  while (k < NI.GetOutDeg()) {
    int v = NI.GetOutNId(k);
    if (prev != v) {
      NeighbourV.Add(v);
      prev = v;
    }
    k += 1;
  }
}
#endif

#if 1
// Rok #6
void GetMergeSortedV(TIntV& NeighbourV, TNGraph::TNodeI NI) {
  int j = 0;
  int k = 0;
  int prev = -1;
  int indeg = NI.GetInDeg();
  int outdeg = NI.GetOutDeg();
  //while (j < NI.GetInDeg() && k < NI.GetOutDeg()) {
  if (indeg > 0  &&  outdeg > 0) {
    int v1 = NI.GetInNId(j);
    int v2 = NI.GetOutNId(k);
    while (1) {
      if (v1 <= v2) {
        if (prev != v1) {
          NeighbourV.Add(v1);
          prev = v1;
        }
        j += 1;
        if (j >= indeg) {
          break;
        }
        v1 = NI.GetInNId(j);
      } else {
        if (prev != v2) {
          NeighbourV.Add(v2);
          prev = v2;
        }
        k += 1;
        if (k >= outdeg) {
          break;
        }
        v2 = NI.GetOutNId(k);
      }
    }
  }
  while (j < indeg) {
    int v = NI.GetInNId(j);
    if (prev != v) {
      NeighbourV.Add(v);
      prev = v;
    }
    j += 1;
  }
  while (k < outdeg) {
    int v = NI.GetOutNId(k);
    if (prev != v) {
      NeighbourV.Add(v);
      prev = v;
    }
    k += 1;
  }
}
#endif

#if 0
// Arijit #1
//Count Triangles time (elapsed): 165.643402, cpu: 1793.532704
//Count Triangles time (elapsed): 167.957260, cpu: 1808.102704
//Count Triangles time (elapsed): 170.232483, cpu: 1765.132704
int GetCommon(TIntV& A, TIntV& B) {
  int i = 0, j = 0;
  int ret = 0;
  while (i < A.Len()) {
    while (j < B.Len() && B[j] < A[i]) {
      j += 1;
    }
    if (j == B.Len()) {
      break;
    }
    if (B[j] == A[i]) ret += 1;
    i += 1;
  }
  return ret;
}
#endif

#if 0
// Rok1 #2
//Count Triangles time (elapsed): 156.530090, cpu: 1903.582704
//Count Triangles time (elapsed): 164.354095, cpu: 1780.882704
//Count Triangles time (elapsed): 172.976929, cpu: -2146.484592
int GetCommon(TIntV& A, TIntV& B) {
  int i, j;
  int ret = 0;
  int alen, blen;
  TInt ai;

  alen = A.Len();
  blen = B.Len();
  j = 0;
  for (i = 0; i < alen; i++) {
    ai = A[i];
    for ( ; j < blen && B[j] < ai; j++) {
    }
    if (j == blen) {
      break;
    }
    if (B[j] == ai) ret++;
  }
  return ret;
}
#endif

#if 1
// Rok2 #3
//Count Triangles time (elapsed): 155.195999, cpu: 1269.432704
//Count Triangles time (elapsed): 163.985474, cpu: 1260.712704
//Count Triangles time (elapsed): 164.699173, cpu: 1261.302704
int GetCommon(TIntV& A, TIntV& B) {
  int i, j;
  int ret = 0;
  int alen, blen;
  int d;
  TInt ai;

  alen = A.Len();
  blen = B.Len();
  i = 0;
  j = 0;
  if (i >= alen  ||  j >= blen) {
    return ret;
  }

  while (1) {
    d = A[i] - B[j];
    if (d < 0) {
      i++;
      if (i >= alen) {
        break;
      }
    } else if (d > 0) {
      j++;
      if (j >= blen) {
        break;
      }
    } else {
      ret++;
      i++;
      if (i >= alen) {
        break;
      }
      j++;
      if (j >= blen) { 
        break;
      }
    }
  }
  return ret;
}
#endif

#if 0
// Eric #4
//Count Triangles time (elapsed): 166.162323, cpu: 2048.942704
//Count Triangles time (elapsed): 159.984497, cpu: 1769.572704
//Count Triangles time (elapsed): 167.080368, cpu: 1727.222704
int GetCommon(TIntV& A, TIntV& B) {
  int ret = 0;
  int i = 0;
  int j = 0;
  int alen, blen;

  alen = A.Len();
  blen = B.Len();
  while (i < alen && j < blen) {
    while (i < alen && A[i] < B[j]) {
      i++;
    }
    // Optional check
    if (i == alen) {
      break;
    }

    while (j < blen && A[i] > B[j]) {
      j++;
    }
    // Optional check
    if (j == blen) {
      break;
    }

    if (A[i] == B[j]) {
      ret++;
      i++;
      j++;
    }
  }

  return ret;
}
#endif

}
