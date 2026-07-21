//
// Created by Kyle Smith on 2026-06-11.
//

#ifndef POINTMAPPER_NODE_H
#define POINTMAPPER_NODE_H

#include <vector>
#include <webgpu/webgpu.h>

#include "Node.h"



namespace pointmapper::pipeline {
    struct PipelineBundle {
        WGPUCommandEncoder cmd;
        WGPUComputePassEncoder encoder;
        WGPUQueue queue;
        WGPUDevice device;

        void EndComputePass() {
            if (encoder != nullptr) {
                wgpuComputePassEncoderEnd(encoder);
                encoder = nullptr;
            }
        }

        void Flush() {
            if (encoder != nullptr) {
                wgpuComputePassEncoderEnd(encoder);
            }
            auto buf = wgpuCommandEncoderFinish(cmd, nullptr);
            wgpuQueueSubmit(queue, 1, &buf);
            wgpuCommandBufferRelease(buf);

            if (encoder != nullptr) {
                wgpuComputePassEncoderRelease(encoder);
                encoder = nullptr;
            }
            wgpuCommandEncoderRelease(cmd);

            cmd = wgpuDeviceCreateCommandEncoder(device, nullptr);
            encoder = wgpuCommandEncoderBeginComputePass(cmd, nullptr);
        }
    };

    class PointmapperPipeline;

    extern PointmapperPipeline* PIPELINE;

    class OutputBase;
    class InputBase;
    template<typename T>
    class Output;

    template<typename T>
    class Input;

    class Node : public std::enable_shared_from_this<Node> {
    public:
        virtual ~Node() = default;

        std::vector<std::shared_ptr<InputBase>>& GetInputs();
        std::vector<std::shared_ptr<OutputBase>>& GetOutputs();

        void Build();

        /// Allocate buffers and such for outputs
        virtual void Hydrate() = 0;

        void LazyProcess(PipelineBundle& bundle);

        /// Process inputs and produce outputs
        virtual void Process(PipelineBundle&) = 0;

        [[nodiscard]] bool WasBuilt() const {
            return ready;
        }

        /// If true, Process() will only be called if an input has a new value.
        bool ProcessLazily = true;
    protected:
        template<typename T> std::shared_ptr<Output<T>> CreateOutput() {
            auto result = std::make_shared<Output<T>>();
            outputs.push_back(result);
            return result;
        }

        template<typename T> std::shared_ptr<Input<T>> CreateInput() {
            auto result = std::make_shared<Input<T>>(this);
            inputs.push_back(result);
            return result;
        }
    private:
        std::vector<std::shared_ptr<InputBase>> inputs;
        std::vector<std::shared_ptr<OutputBase>> outputs;

        bool ready = false;
    };

    class InputBase {
    public:
        InputBase(Node* a) {
            node = a;
        }
        virtual ~InputBase() = default;
        virtual std::shared_ptr<OutputBase> GetSource() = 0;
        virtual bool IsConnected() = 0;

        [[nodiscard]] std::shared_ptr<Node> GetNode() const {
            return node->shared_from_this();
        }

        void Notify() {
            newData = true;
        }

        void Reset() {
            newData = false;
        }

        [[nodiscard]] bool HasNewData() const {
            return newData;
        }
    private:
        Node* node;
        bool newData = false;
    };

    class OutputBase {
    public:
        virtual ~OutputBase() = default;

        virtual std::vector<InputBase*> GetTargets() = 0;

        void NotifyAll() {
            for (auto target : GetTargets()) {
                target->Notify();
            }
        }
    };

    template<typename T>
    class Input : public InputBase {
    public:
        Input(Node* a) : InputBase(a) {}

        std::shared_ptr<OutputBase> GetSource() override {
            return source;
        }

        bool IsConnected() override {
            return source != nullptr && source->ready;
        }

        T* operator ->() {
            return &source->value;
        }

        void Connect(std::shared_ptr<Output<T>> src) {
            source = src;
            src->AddConsumer(this);
        }

    private:
        std::shared_ptr<Output<T>> source;
    };

    template<typename T>
    class Output : public OutputBase {
    public:
        T* operator ->() {
            return &value;
        }

        T& operator*() {
            return value;
        }

        std::vector<InputBase*> GetTargets() override {
            return inputs;
        }

        void MarkReady() {
            ready = true;
        }
    private:
        friend class Input<T>;

        void AddConsumer(Input<T>* consumer) {
            inputs.push_back(consumer);
        }

        T value;
        std::vector<InputBase*> inputs;

        bool ready = false;
    };
}

#endif //POINTMAPPER_NODE_H
