﻿using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System;

namespace ACT
{
    public class ActBlendAddAttr : ActBuffInterval
    {
        [SerializeField]
        public EAttr       AttrID           = EAttr.AP;
        [SerializeField]
        public EAttr       AttrPercentRefer = EAttr.AP;
        [SerializeField]
        public List<float> TickPercents     = new List<float>();
        [SerializeField]
        public List<Int32> TickFixValues    = new List<int>();

        public float       CurrTickTime
        {
            get; private set;
        }

        public Int32       CurrTickIndex
        {
            get; private set;
        }

        public float       CurrPercent
        {
            get; private set;
        }

        public Int32       CurrFixValue
        {
            get; private set;
        }

        public override void Enter()
        {
            base.Enter();
            if (PlayOnTrigger)
            {
                this.Trigger();
            }
            this.CurrTickTime = Time.realtimeSinceStartup;
        }

        public override void Execute()
        {
            if (Time.realtimeSinceStartup - this.CurrTickTime > TickInterval)
            {
                this.Trigger();
                this.CurrTickTime = Time.realtimeSinceStartup;
            }
        }

        public override void Stop()
        {
            CharacterHelper.CalcAddAttr(Carryer, AttrID, AttrPercentRefer, CurrPercent, CurrFixValue, false);
        }

        public override void Exit()
        {
            CharacterHelper.CalcAddAttr(Carryer, AttrID, AttrPercentRefer, CurrPercent, CurrFixValue, false);
        }

        public override void Overlay()
        {

        }

        void Trigger()
        {
            float tickPer = TickPercents.Count >= (CurrTickIndex + 1) ?  TickPercents[CurrTickIndex] : 0;
            Int32 tickFix = TickFixValues.Count >= (CurrTickIndex + 1) ? TickFixValues[CurrTickIndex] : 0;
            CharacterHelper.CalcAddAttr(Carryer, AttrID, AttrPercentRefer, CurrPercent, CurrFixValue, false);
            CharacterHelper.CalcAddAttr(Carryer, AttrID, AttrPercentRefer, tickPer, tickFix, true);
            this.CurrTickIndex++;
            this.CurrPercent = tickPer;
            this.CurrFixValue = tickFix;
        }
    }
}

