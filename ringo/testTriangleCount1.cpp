// Testing Triangle Count function

#include "../snap-core/Snap.h"

int main(int argc, char* argv[]) {
  int i;
  TTableContext Context;

  //char filename[500] = "/dfs/ilfs2/0/ringo/benchmarks/soc-LiveJournal1.graph";
  //char filename[500] = "/dfs/ilfs2/0/ringo/benchmarks/twitter_rv.graph";
  char filename[500] = "twitter_rv.graph";

  if (argc >= 2){
    strcpy(filename,argv[1]);
  }
  struct timeval start, end;
  float delta;
  TTmProfiler Profiler;
  int TimerId = Profiler.AddTimer("Profiler");

  Profiler.ResetTimer(TimerId);
  Profiler.StartTimer(TimerId);
  gettimeofday(&start, NULL);
  TFIn FIn(filename);
  PNGraph Q = TNGraph::Load(FIn);
  Profiler.StopTimer(TimerId);
  gettimeofday(&end, NULL);
  delta = ((end.tv_sec  - start.tv_sec) * 1000000u + 
            end.tv_usec - start.tv_usec) / 1.e6;
  printf("__load__\ttime %7.3f\tcpu %8.3f\n", delta, Profiler.GetTimerSec(TimerId));
  
  //for (i = 0; i < 10; i++) {
  for (i = 0; i < 1; i++) {
    PNGraph Graph = Q;
    THashMP <TInt, TInt> H(Graph->GetNodes());
    TIntV MapV;

    Profiler.ResetTimer(TimerId);
    Profiler.StartTimer(TimerId);
    gettimeofday(&start, NULL);

    int ind = 0;
    for (TNGraph::TNodeI NI = Graph->BegNI(); NI < Graph->EndNI(); NI++)   {
      H.AddDat(NI.GetId(), ind);
      MapV.Add(NI.GetId());
      ind += 1;
    }

    gettimeofday(&end, NULL);
    Profiler.StopTimer(TimerId);
    delta = ((end.tv_sec  - start.tv_sec) * 1000000u +
            end.tv_usec - start.tv_usec) / 1.e6;
    printf("__nodemap__\ttime %7.3f\tcpu %8.3f\n", delta, Profiler.GetTimerSec(TimerId));

    Profiler.ResetTimer(TimerId);
    Profiler.StartTimer(TimerId);
    gettimeofday(&start, NULL);

    int64 cnt = TSnap::CountTriangles(Graph, H, MapV);
    Profiler.StopTimer(TimerId);
    gettimeofday(&end, NULL);
    delta = ((end.tv_sec  - start.tv_sec) * 1000000u + 
            end.tv_usec - start.tv_usec) / 1.e6;
    printf("__triads__\ttime %7.3f\tcpu %8.3f\n", delta, Profiler.GetTimerSec(TimerId));
    printf("triangles %s\n", TUInt64::GetStr(cnt).CStr());
  }

  return 0;
}

