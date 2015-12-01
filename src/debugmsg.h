#pragma once

template<typename Type>
class MessageClass {
public:
   typedef Type IdType;

   virtual ~MessageClass() = default;

   virtual Type type() const = 0;

   bool isA(Type typeId) const {
      return type() == typeId;
   }
};

template<typename MessageType, typename MessageType::IdType TypeId>
class MessageClassBase : public MessageType {
public:
   typename MessageType::IdType type() const override {
      return TypeId;
   }

};
