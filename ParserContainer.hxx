#ifndef ParserContainer_hxx
#define ParserContainer_hxx

#include <sipstack/HeaderFieldValueList.hxx>
#include <sipstack/ParserContainerBase.hxx>

namespace Vocal2
{

template<class T>
class ParserContainer : public ParserContainerBase
{
   public:
      ParserContainer() : mList(new HeaderFieldValueList) 
      {
      }
      
      ParserContainer(HeaderFieldValueList* list)
         : mList(list)
      {
         HeaderFieldValue* it = mList->front();
         while (it != 0)
         {
            it->setParserCategory(new T(it));
            it = it->next;
         }
      }
      
      ParserContainer& operator=(const ParserContainer& other)
      {
         assert(0);
      }
      
      
      bool empty() const { return (mList->first == 0); }
      void clear() { assert(0); }
      
      T& front() { return *dynamic_cast<T*>(mList->first->getParserCategory()); }
      T& back() { return *dynamic_cast<T*>(mList->last->getParserCategory()); }
      
      void push_front(T & t) { mList->push_front(new HeaderFieldValue(t.clone())); }
      void push_back(T & t) { mList->push_front(new HeaderFieldValue(t.clone())); }
      
      void pop_front(T & t) { mList->pop_front(); }
      void pop_back(T & t) { mList->pop_back(); }
      
      ParserContainer reverse();
      
      int size() const { assert(0); return 0; }
      
      class Iterator
      {
         private:
            Iterator(HeaderFieldValue* p);
         public:
            Iterator& operator++(int)
            {
               Iterator tmp(mPtr);
               mPtr = mPtr->next;
               return tmp;
            }
            
            Iterator& operator++()
            {
               mPtr = mPtr->next;
               return this;
            }
            
            T& operator*()
            {
               return *dynamic_cast<T*>(mPtr->parserCategory);
            }
            
            T& operator->()
            {
               return *dynamic_cast<T*>(mPtr->parserCategory);
            }
            
            bool operator==(Iterator& i)
            {
               return mPtr == i.mPtr;
            }
         private:
            HeaderFieldValue* mPtr;
      };
      
      Iterator begin() { return Iterator(mList->first); }
      Iterator end() { return Iterator(0); }
      typedef Iterator iterator;

      virtual ParserContainerBase* clone(HeaderFieldValueList* hfvs) const
      {
         return new ParserContainer(hfvs);
      }

      virtual std::ostream& encode(std::ostream& str) const
      {
         if (mList->first)
         {
            HeaderFieldValue* hfv = mList->first;
            do
            {
               hfv->encode(str);
               str << Symbols::CRLF;
               hfv = hfv->next;
            } while (hfv != 0);
         }
         return str;
      }
      
   protected:
      virtual void parser() {}

   private:
      HeaderFieldValueList* mList;
};
 
}







#endif
