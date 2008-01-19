
/*
* Delta3D Open Source Game and Simulation Engine
*
* This library is free software; you can redistribute it and/or modify it under
* the terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; either version 2.1 of the License, or (at your option)
* any later version.
*
* This library is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
* details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this library; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*
* Bradley Anderegg 01/17/2008
*/


#ifndef DELTA_TEMPLATE_UTILITY
#define DELTA_TEMPLATE_UTILITY

namespace dtUtil
{

   /**
    * This odd functor is actually useful for supplying a default 
    * template parameter when a functors is expected
    */
   template<typename Ret = void, typename T = void>
   struct DoNothing
   {
      Ret operator()(T)
      {

      }
   };

   //This functor uses partial template specialization to invoke a 
   //functor or first class object generically when the templated 
   //functor type requires no specific traits.
   template <typename FunctorType, typename ArgType, typename RetType = void>
   struct EvaluateFunctor
   {
      RetType operator()(FunctorType func, ArgType arg)
      {
         return func(arg);
      }
   };

   template <typename FunctorType, typename ArgType, typename RetType>
   struct EvaluateFunctor<FunctorType*, ArgType, RetType>
   {
      RetType operator()(FunctorType* func, ArgType arg)
      {
         return func->operator()(arg);
      }
   };

   //TODO- Allow Evaluate to work without forcing an argument to the operator()
   //template <typename FunctorType, typename RetType>
   //struct EvaluateFunctor
   //{
   //   RetType operator()(FunctorType func)
   //   {
   //      return func();
   //   }
   //};

   //TODO- Allow Evaluate to work without forcing an argument to the operator()
   //template <typename FunctorType, typename RetType>
   //struct EvaluateFunctor<FunctorType*, RetType>
   //{
   //   RetType operator()(FunctorType* func)
   //   {
   //      return func->operator()();
   //   }
   //};

   template <typename CompareHandler, typename InvokeHandler, typename EvaluateResult>
   struct EvaluateInvoke
   {
      void operator()(std::pair<CompareHandler, InvokeHandler>& dataType)
      {
         dtUtil::EvaluateFunctor<CompareHandler, EvaluateResult, bool> eval;
         EvaluateResult result;

         if(eval(dataType.first, result))
         {
            dtUtil::EvaluateFunctor<InvokeHandler, EvaluateResult> triggerHandler;
            triggerHandler(dataType.second, result);
         }
      }
   };

   //TODO- Allow Evaluate to work without forcing an argument for EvaluateResult
   //template <typename CompareHandler, typename InvokeHandler, typename EvaluateResult>
   //struct EvaluateInvoke<CompareHandler, InvokeHandler, void>
   //{
   //   void operator()(std::pair<CompareHandler, InvokeHandler>& dataType)
   //   {
   //      dtUtil::EvaluateFunctor<CompareHandler, bool> eval;
   //      if(eval(dataType.first))
   //      {
   //         dtUtil::EvaluateFunctor<InvokeHandler, void> triggerHandler;
   //         triggerHandler(dataType.second);
   //      }
   //   }
   //};

}//namespace dtUtil

#endif//DELTA_TEMPLATE_UTILITY
