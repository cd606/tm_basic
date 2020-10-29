using System;
using Dev.CD606.TM.Infra;
using Dev.CD606.TM.Infra.RealTimeApp;
using Here;

namespace Dev.CD606.TM.Basic
{
    public static class CommonFlowUtils<Env> where Env : EnvBase
    {
        public static Func<TimedDataWithEnvironment<Env,T>,Option<TimedDataWithEnvironment<Env,T>>> idFunc<T>()
        {
            return KleisliUtils<Env>.liftPure((T t) => t);
        }
        public static AbstractAction<Env,T,T> idFuncAction<T>(bool threaded=false)
        {
            return RealTimeAppUtils<Env>.kleisli(idFunc<T>(), threaded);
        }
    }
}