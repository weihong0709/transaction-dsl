/*
 * EventHandlerRegistry.cc
 *
 * Created on: Apr 29, 2013
 *     author: Darwin Yuan
 *
 * Copyright 2013 ThoughtWorks, All Rights Reserved.
 *
 */ 

#include "trans-dsl/utils/EventHandlerRegistry.h"
#include "event/concept/Event.h"
#include <new>

////////////////////////////////////////////////////////////////////
EventHandlerRegistry::EventHandler::EventHandler
        ( const EventId eventId
        , DummyEventHandlerClass* thisPointer
        , DummyEventHandler handler)
            : thisPointer(thisPointer)
            , handler(handler)
            , eventId(eventId)
{
}

////////////////////////////////////////////////////////////////////
EventHandlerRegistry::EventHandler::EventHandler
        ( const EventId eventId)
            : thisPointer(0)
            , handler(&DummyEventHandlerClass::dummy)
            , eventId(eventId)
{
}

////////////////////////////////////////////////////////////////////
EventHandlerRegistry::EventHandler::EventHandler(const EventHandler& rhs)
   : thisPointer(rhs.thisPointer)
   , handler(rhs.handler)
   , eventId(rhs.eventId)
{
}

////////////////////////////////////////////////////////////////////
bool EventHandlerRegistry::EventHandler::matches(const Event& event) const
{
   return event.matches(eventId);
}

EventHandlerRegistry::EventHandler::~EventHandler()
{
   thisPointer = 0;
   handler = 0;
   eventId = 0;
}

////////////////////////////////////////////////////////////////////
EventHandlerRegistry::Handler::Handler() : forever(false), used(false)
{}

////////////////////////////////////////////////////////////////////
void* EventHandlerRegistry::Handler::alloc(bool forever)
{
   this->used = true;
   this->forever = forever;
   return handler.alloc();
}

////////////////////////////////////////////////////////////////////
bool EventHandlerRegistry::Handler::isFree() const
{
   return not used;
}

////////////////////////////////////////////////////////////////////
void EventHandlerRegistry::Handler::clear()
{
   if(used and not forever)
   {
      handler->~EventHandler();
      used = false;
   }
}

////////////////////////////////////////////////////////////////////
void EventHandlerRegistry::Handler::reset()
{
   if(used)
   {
      handler->~EventHandler();
      used = false;
      forever = false;
   }
}

////////////////////////////////////////////////////////////////////
bool EventHandlerRegistry::Handler::matches(const Event& event) const
{
   if(not used) return false;

   return handler->matches(event);
}

////////////////////////////////////////////////////////////////////
Status EventHandlerRegistry::EventHandler::handleEvent(const Event& event,
         EventHandlerExecuter& executer)
{
   // It's an untouched event.
   if(thisPointer == 0)
   {
      return SUCCESS;
   }

   event.consume();
   return executer.exec(event, thisPointer, handler);
}

int EventHandlerRegistry::allocSlot() const
{
   for(int i=0; i<MAX_EVENT_HANDLER; i++)
   {
      if(handlers[i].isFree())
      {
         return i;
      }
   }

   return -1;
}

Status EventHandlerRegistry::addUntouchedEvent(const EventId eventId)
{
   int slot = allocSlot();
   if(slot < 0)
   {
      return OUT_OF_SCOPE;
   }

   new (handlers[slot].alloc()) EventHandler(eventId);

   return SUCCESS;
}

////////////////////////////////////////////////////////////////////
Status EventHandlerRegistry::doAddHandler(const EventId eventId,
         DummyEventHandlerClass* thisPointer, DummyEventHandler handler, bool forever)
{
   int slot = allocSlot();
   if(slot < 0)
   {
      return OUT_OF_SCOPE;
   }

   new (handlers[slot].alloc(forever)) EventHandler(eventId, thisPointer, handler);

   return SUCCESS;
}

////////////////////////////////////////////////////////////////////
Status EventHandlerRegistry::handleEvent(const Event& event,
         EventHandlerExecuter& executer)
{
   for (int i = 0; i < MAX_EVENT_HANDLER; i++)
   {
      if(handlers[i].matches(event))
      {
         EventHandler handler = *(handlers[i].handler);
         clear();
         return handler.handleEvent(event, executer);
      }
   }

   return UNKNOWN_EVENT;
}

////////////////////////////////////////////////////////////////////
void EventHandlerRegistry::clear()
{
   for (int i = 0; i < MAX_EVENT_HANDLER; i++)
   {
      handlers[i].clear();
   }
}

////////////////////////////////////////////////////////////////////
void EventHandlerRegistry::reset()
{
   for (int i = 0; i < MAX_EVENT_HANDLER; i++)
   {
      handlers[i].reset();
   }
}
