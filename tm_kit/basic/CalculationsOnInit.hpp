#ifndef TM_KIT_BASIC_CALCULATIONS_ON_INIT_HPP_
#define TM_KIT_BASIC_CALCULATIONS_ON_INIT_HPP_

#include <tm_kit/infra/RealTimeApp.hpp>
#include <tm_kit/infra/SinglePassIterationApp.hpp>
#include <tm_kit/infra/Environments.hpp>
#include <type_traits>

namespace dev { namespace cd606 { namespace tm { namespace basic {

    //Importers for calculating value on init

    //Please notice that Func takes only one argument (the logger)
    //This is deliberate choice. The reason is that if Func needs anything
    //inside Env, it should be able to capture that by itself outside this 
    //logic. If we force Env to maintain something for use of Func, which is
    //only executed at startup, there will be trickly resource-release timing
    //questions, and by not doing that, we allow Func to manage resources by
    //itself.

    template <class App, class Func>
    class ImporterOfValueCalculatedOnInit {};

    template <class Env, class Func>
    class ImporterOfValueCalculatedOnInit<infra::template RealTimeApp<Env>, Func> 
        : public infra::template RealTimeApp<Env>::template AbstractImporter<
            decltype((* ((Func *) nullptr))(
                std::function<void(infra::LogLevel, std::string const &)>()
            ))
            >
    {
    private:
        Func f_;
    public:
        using OutputT = decltype((* ((Func *) nullptr))(
                std::function<void(infra::LogLevel, std::string const &)>()
            ));
        ImporterOfValueCalculatedOnInit(Func &&f) : f_(std::move(f)) {}
        virtual void start(Env *env) override final {
            this->publish(env, f_(
                [env](infra::LogLevel level, std::string const &s) {
                    env->log(level, s);
                }
            ));
        }
    };
    
    template <class Env, class Func>
    class ImporterOfValueCalculatedOnInit<infra::template SinglePassIterationApp<Env>, Func> 
        : public infra::template SinglePassIterationApp<Env>::template AbstractImporter<
            decltype((* ((Func *) nullptr))(
                std::function<void(infra::LogLevel, std::string const &)>()
            ))
            >
    {
    public:
        using OutputT = decltype((* ((Func *) nullptr))(
                std::function<void(infra::LogLevel, std::string const &)>()
            ));
    private:
        Func f_;
        typename infra::template SinglePassIterationApp<Env>::template Data<OutputT> data_;
    public:
        ImporterOfValueCalculatedOnInit(Func &&f) : f_(std::move(f)), data_(std::nullopt) {}
        virtual void start(Env *env) override final {
            data_ = infra::template SinglePassIterationApp<Env>::template pureInnerData<OutputT>(
                env
                , f_(
                    [env](infra::LogLevel level, std::string const &s) {
                        env->log(level, s);
                    }
                )
                , true
            );
        }
        virtual typename infra::template SinglePassIterationApp<Env>::template Data<OutputT> generate() override final {
            return data_;
        }
    };

    template <class App, class Func>
    auto importerOfValueCalculatedOnInit(Func &&f)
        -> std::shared_ptr<typename App::template Importer<
            typename ImporterOfValueCalculatedOnInit<App,Func>::OutputT
        >> 
    {
        return App::importer(
            new ImporterOfValueCalculatedOnInit<App,Func>(std::move(f))
        );
    }

    //on order facilities for holding pre-calculated value and returning it on query

    template <class App, class Req, class PreCalculatedResult>
    class LocalOnOrderFacilityReturningPreCalculatedValue : 
        public App::template AbstractIntegratedLocalOnOrderFacility<Req, PreCalculatedResult, PreCalculatedResult>
    {
    private:
        std::conditional_t<App::PossiblyMultiThreaded, std::mutex, bool> mutex_;
        PreCalculatedResult preCalculatedRes_;
    public:
        LocalOnOrderFacilityReturningPreCalculatedValue()
            : mutex_(), preCalculatedRes_() 
        {}
        virtual void start(typename App::EnvironmentType *) {}
        virtual void handle(typename App::template InnerData<typename App::template Key<Req>> &&req) override final {
            std::conditional_t<App::PossiblyMultiThreaded, std::lock_guard<std::mutex>, bool> _(mutex_);
            this->publish(
                req.environment
                , typename App::template Key<PreCalculatedResult> {
                    req.timedData.value.id(), preCalculatedRes_
                }
                , true
            );
        }
        virtual void handle(typename App::template InnerData<PreCalculatedResult> &&data) override final {
            std::conditional_t<App::PossiblyMultiThreaded, std::lock_guard<std::mutex>, bool> _(mutex_);
            preCalculatedRes_ = std::move(data.timedData.value);
        }
    };

    template <class App, class Req, class PreCalculatedResult>
    auto localOnOrderFacilityReturningPreCalculatedValue()
        -> std::shared_ptr<typename App::template LocalOnOrderFacility<Req, PreCalculatedResult, PreCalculatedResult>>
    {
        return App::localOnOrderFacility(new LocalOnOrderFacilityReturningPreCalculatedValue<App,Req,PreCalculatedResult>());
    }

    template <class App, class Req, class PreCalculatedResult, class Func>
    class LocalOnOrderFacilityUsingPreCalculatedValue : 
        public App::template AbstractIntegratedLocalOnOrderFacility<
            Req
            , decltype(
                (* ((Func *) nullptr))(
                    * ((PreCalculatedResult const *) nullptr)
                    , * ((Req const *) nullptr)
                )
            )
            , PreCalculatedResult
        >
    {
    private:
        Func f_;
        std::conditional_t<App::PossiblyMultiThreaded, std::mutex, bool> mutex_;
        PreCalculatedResult preCalculatedRes_;
    public:
        using OutputT = decltype(
                (* ((Func *) nullptr))(
                    * ((PreCalculatedResult const *) nullptr)
                    , * ((Req const *) nullptr)
                )
            );
        LocalOnOrderFacilityUsingPreCalculatedValue(Func &&f) :
            f_(std::move(f)), mutex_(), preCalculatedRes_() 
        {}
        virtual void start(typename App::EnvironmentType *) override final {}
        virtual void handle(typename App::template InnerData<typename App::template Key<Req>> &&req) override final {
            std::conditional_t<App::PossiblyMultiThreaded, std::lock_guard<std::mutex>, bool> _(mutex_);
            this->publish(
                req.environment
                , typename App::template Key<OutputT> {
                    req.timedData.value.id(), f_(preCalculatedRes_, req.timedData.value.key())
                }
                , true
            );
        }
        virtual void handle(typename App::template InnerData<PreCalculatedResult> &&data) override final {
            std::conditional_t<App::PossiblyMultiThreaded, std::lock_guard<std::mutex>, bool> _(mutex_);
            preCalculatedRes_ = std::move(data.timedData.value);
        }
    };

    template <class App, class Req, class PreCalculatedResult, class Func>
    auto localOnOrderFacilityUsingPreCalculatedValue(Func &&f)
        -> std::shared_ptr<typename App::template LocalOnOrderFacility<Req, typename LocalOnOrderFacilityUsingPreCalculatedValue<App,Req,PreCalculatedResult,Func>::OutputT, PreCalculatedResult>>
    {
        return App::localOnOrderFacility(new LocalOnOrderFacilityUsingPreCalculatedValue<App,Req,PreCalculatedResult,Func>(std::move(f)));
    }

    //combination of the two above, on order facilities for 
    //holding pre-calculated value which is calculated internally
    //at init, and returning it on query

    template <class App, class Req, class CalcFunc>
    class OnOrderFacilityReturningInternallyPreCalculatedValue : 
        public virtual App::IExternalComponent
        , public App::template AbstractOnOrderFacility<
            Req
            , decltype((* ((CalcFunc *) nullptr))(
                std::function<void(infra::LogLevel, std::string const &)>()
            ))
        >
    {
    public:
        using PreCalculatedResult = decltype((* ((CalcFunc *) nullptr))(
                std::function<void(infra::LogLevel, std::string const &)>()
            ));
    private:
        CalcFunc f_;
        PreCalculatedResult preCalculatedRes_;
    public:
        OnOrderFacilityReturningInternallyPreCalculatedValue(CalcFunc &&f)
            : f_(f), preCalculatedRes_() 
        {}
        virtual void start(typename App::EnvironmentType *env) override final {
            preCalculatedRes_ = f_(
                [env](infra::LogLevel level, std::string const &s) {
                    env->log(level, s);
                }
            );
        }
        virtual void handle(typename App::template InnerData<typename App::template Key<Req>> &&req) override final {
            this->publish(
                req.environment
                , typename App::template Key<PreCalculatedResult> {
                    req.timedData.value.id(), preCalculatedRes_
                }
                , true
            );
        }
    };

    template <class App, class Req, class CalcFunc>
    auto onOrderFacilityReturningInternallyPreCalculatedValue(CalcFunc &&f)
        -> std::shared_ptr<typename App::template OnOrderFacility<Req, typename OnOrderFacilityReturningInternallyPreCalculatedValue<App,Req,CalcFunc>::PreCalculatedResult>>
    {
        return App::fromAbstractOnOrderFacility(new OnOrderFacilityReturningInternallyPreCalculatedValue<App,Req,CalcFunc>(std::move(f)));
    }

    template <class App, class Req, class CalcFunc, class FetchFunc>
    class OnOrderFacilityUsingInternallyPreCalculatedValue : 
        public virtual App::IExternalComponent
        , public App::template AbstractOnOrderFacility<
            Req
            , decltype(
                (* ((FetchFunc *) nullptr))(
                    * ((typename OnOrderFacilityReturningInternallyPreCalculatedValue<App,Req,CalcFunc>::PreCalculatedResult const *) nullptr)
                    , * ((Req const *) nullptr)
                )
            )
        >
    {
    public:
        using PreCalculatedResult = decltype((* ((CalcFunc *) nullptr))(
                std::function<void(infra::LogLevel, std::string const &)>()
            ));
        using OutputT = decltype(
                (* ((FetchFunc *) nullptr))(
                    * ((PreCalculatedResult const *) nullptr)
                    , * ((Req const *) nullptr)
                )
            );
    private:
        CalcFunc calcFunc_;
        FetchFunc fetchFunc_;
        PreCalculatedResult preCalculatedRes_;
    public:
        OnOrderFacilityUsingInternallyPreCalculatedValue(CalcFunc &&calcFunc, FetchFunc &&fetchFunc) :
            calcFunc_(std::move(calcFunc)), fetchFunc_(std::move(fetchFunc)), preCalculatedRes_() 
        {}
        virtual void start(typename App::EnvironmentType *env) override final {
            preCalculatedRes_ = calcFunc_(
                [env](infra::LogLevel level, std::string const &s) {
                    env->log(level, s);
                }
            );
        }
        virtual void handle(typename App::template InnerData<typename App::template Key<Req>> &&req) override final {
            this->publish(
                req.environment
                , typename App::template Key<OutputT> {
                    req.timedData.value.id(), fetchFunc_(preCalculatedRes_, req.timedData.value.key())
                }
                , true
            );
        }
    };

    template <class App, class Req, class CalcFunc, class FetchFunc>
    auto onOrderFacilityUsingInternallyPreCalculatedValue(CalcFunc &&calcFunc, FetchFunc &&fetchFunc)
        -> std::shared_ptr<typename App::template OnOrderFacility<Req, typename OnOrderFacilityUsingInternallyPreCalculatedValue<App,Req,CalcFunc,FetchFunc>::OutputT>>
    {
        return App::fromAbstractOnOrderFacility(new OnOrderFacilityUsingInternallyPreCalculatedValue<App,Req,CalcFunc,FetchFunc>(std::move(calcFunc), std::move(fetchFunc)));
    }

} } } } 

#endif