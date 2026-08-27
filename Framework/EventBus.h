#pragma once

#include <typeindex>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>

// 타입 세이프 제네릭 이벤트 버스 (Observer Pattern / Event-Driven Architecture)
class EventBus
{
public:
    static EventBus& Get()
    {
        static EventBus instance;
        return instance;
    }

private:
    class IHandlerWrapper
    {
    public:
        virtual ~IHandlerWrapper() = default;
    };

    template <typename EventType>
    class HandlerWrapper : public IHandlerWrapper
    {
    public:
        explicit HandlerWrapper(std::function<void(const EventType&)> handler)
            : m_handler(std::move(handler))
        {
        }

        void Execute(const EventType& event) const
        {
            if (m_handler)
            {
                m_handler(event);
            }
        }

    private:
        std::function<void(const EventType&)> m_handler;
    };

public:
    EventBus() = default;
    ~EventBus() = default;
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    // 이벤트 구독 (Subscribe)
    template <typename EventType>
    void Subscribe(std::function<void(const EventType&)> handler)
    {
        std::type_index typeId = std::type_index(typeid(EventType));
        m_subscribers[typeId].push_back(std::make_shared<HandlerWrapper<EventType>>(std::move(handler)));
    }

    // 이벤트 발행 (Publish / Broadcast)
    template <typename EventType>
    void Publish(const EventType& event)
    {
        std::type_index typeId = std::type_index(typeid(EventType));
        auto it = m_subscribers.find(typeId);
        if (it != m_subscribers.end())
        {
            for (const auto& wrapper : it->second)
            {
                auto specificHandler = std::static_pointer_cast<HandlerWrapper<EventType>>(wrapper);
                if (specificHandler)
                {
                    specificHandler->Execute(event);
                }
            }
        }
    }

    // 모든 리스너 초기화
    void Clear()
    {
        m_subscribers.clear();
    }

private:
    std::unordered_map<std::type_index, std::vector<std::shared_ptr<IHandlerWrapper>>> m_subscribers;
};