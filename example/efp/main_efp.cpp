#include <cadmium/core/modeling/atomic.hpp>
#include <cadmium/core/modeling/coupled.hpp>
#include <cadmium/core/simulation/coordinator.hpp>
#include <iostream>
#include <limits>
#include <string>

struct Job {
    int id;
    double timeGenerated, timeProcessed;
    explicit Job(int id, double timeGenerated) : id(id), timeGenerated(timeGenerated), timeProcessed(std::numeric_limits<double>::infinity()) {}
};

struct GeneratorState {
    double clock, sigma;
    int jobCount;
    GeneratorState() : clock(), sigma(), jobCount() {}
};

class Generator: public cadmium::Atomic<GeneratorState> {
  private:
    double period;
    std::shared_ptr<cadmium::Port<bool>> stop;
    std::shared_ptr<cadmium::Port<Job>> out;
  public:
    Generator(std::string id, double period): cadmium::Atomic<GeneratorState>(std::move(id), GeneratorState()),
        period(period), stop(std::make_shared<cadmium::Port<bool>>("stop")), out(cadmium::Port<Job>::newPort("out")) {
        addInPort(stop);
        addOutPort(out);
    }

    void internalTransition(GeneratorState& s) const override {
        s.clock += s.sigma;
        s.sigma = period;
        s.jobCount += 1;
    }
    void externalTransition(GeneratorState& s, double e, const cadmium::PortSet& x) const override {
        s.clock += e;
        s.sigma = (stop->getBag().back())? std::numeric_limits<double>::infinity() : std::max(s.sigma - e, 0.);
    }
    void output(const GeneratorState& s, const cadmium::PortSet& y) const override {
        out->addMessage(Job(s.jobCount, s.clock + s.sigma));
    }
    [[nodiscard]] double timeAdvance(const GeneratorState& s) const override {
        return s.sigma;
    }
};

struct ProcessorState {
    double sigma;
    std::shared_ptr<Job> currentJob;
    explicit ProcessorState() : sigma(std::numeric_limits<double>::infinity()), currentJob() {}
};

class Processor: public cadmium::Atomic<ProcessorState> {
 private:
    double processingTime;
 public:
    Processor(std::string id, double processingTime): cadmium::Atomic<ProcessorState>(std::move(id), ProcessorState()),
        processingTime(processingTime) {
        addInPort<Job>("in");
        addOutPort<Job>("out");
    }

    void internalTransition(ProcessorState& s) const override {
        s.sigma = std::numeric_limits<double>::infinity();
        s.currentJob = nullptr;
    }
    void externalTransition(ProcessorState& s, double e, const cadmium::PortSet& x) const override {
        s.sigma -= e;
        if (s.currentJob == nullptr) {
            auto port = x.getPort<Job>("in");
            if (!port->empty()) {
                s.currentJob = port->getBag().back();
                s.sigma = processingTime;
            }
        }
    }
    void output(const ProcessorState& s, const cadmium::PortSet& y) const override {
        y.addMessage("out", *(s.currentJob));
    }
    [[nodiscard]] double timeAdvance(const ProcessorState& s) const override {
        return s.sigma;
    }
};


struct TransducerState {
    double clock;
    double sigma;
    double totalTA;
    int nJobsGenerated;
    int nJobsProcessed;
    explicit TransducerState(double obsTime) : clock(), sigma(obsTime), totalTA(), nJobsGenerated(), nJobsProcessed() {}
};

class Transducer: public cadmium::Atomic<TransducerState> {
 public:
    Transducer(std::string id, double obsTime):
    cadmium::Atomic<TransducerState>(std::move(id), TransducerState(obsTime)) {
        addInPort<Job>("generated");
        addInPort<Job>("processed");
        addOutPort<bool>("stop");
    }

    void internalTransition(TransducerState& s) const override {
        s.clock += s.sigma;
        s.sigma = std::numeric_limits<double>::infinity();

        std::cout << "End time: " << s.clock << std::endl;
        std::cout << "Jobs generated: " << s.nJobsGenerated << std::endl;
        std::cout << "Jobs processed: " << s.nJobsProcessed << std::endl;
        if (s.nJobsProcessed > 0) {
            std::cout << "Average TA: " << s.totalTA / (double) s.nJobsProcessed << std::endl;
        }
        if (s.clock > 0) {
            std::cout << "Throughput: " << (double) s.nJobsProcessed /  s.clock << std::endl;
        }
    }
    void externalTransition(TransducerState& s, double e, const cadmium::PortSet& x) const override {
        s.clock += e;
        s.sigma -= e;
        for (auto& job: x.getPort<Job>("generated")->getBag()) {
            s.nJobsGenerated += 1;
            std::cout << "Job " << job->id << " generated at t = " << s.clock << std::endl;
        }
        for (auto& job: x.getPort<Job>("processed")->getBag()) {
            s.nJobsProcessed += 1;
            job->timeProcessed = s.clock;
            s.totalTA += job->timeProcessed - job->timeGenerated;
            std::cout << "Job " << job->id << " processed at t = " << s.clock << std::endl;
        }
    }
    void output(const TransducerState& s, const cadmium::PortSet& y) const override {
        y.addMessage("stop", true);
    }
    [[nodiscard]] double timeAdvance(const TransducerState& s) const override {
        return s.sigma;
    }
};


 class ExperimentalFrameProcessor: public cadmium::Coupled {
  public:
     explicit ExperimentalFrameProcessor(std::string id, double jobPeriod, double processingTime, double obsTime):
     cadmium::Coupled(std::move(id)) {
		 auto generator = Generator("generator", jobPeriod);
		 auto processor = Processor("processor", processingTime);
         addComponent(generator);
		 addComponent(processor);
		 addComponent(Transducer("transducer", obsTime));

		 addCoupling(generator.getOutPort("out"), processor.getInPort("in"));
		 addInternalCoupling("generator", "out", "processor", "in");
         addInternalCoupling("generator", "out", "transducer", "generated");
         addInternalCoupling("processor", "out", "transducer", "processed");
		 addInternalCoupling("transducer", "stop", "generator", "stop");
     }
 };

 int main() {
     auto model = std::make_shared<ExperimentalFrameProcessor>("efp", 3, 1, 100);
     auto coordinator = cadmium::Coordinator(model, 0);
     coordinator.simulate(std::numeric_limits<double>::infinity());
 }