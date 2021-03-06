#include "strategy_task.h"
 
#include "common.h"

#include "winner_app.h"

const unsigned int cst_max_data_count = 60 * 60 * 4;
 
StrategyTask::StrategyTask(T_TaskInformation &task_info, WinnerApp *app)
    : app_(app)
    , para_(task_info)
    , market_type_(task_info.stock.c_str()[0] == '6' ? TypeMarket::SH : TypeMarket::SZ)
    , quote_data_queue_()
    , tp_start_(Int2Qtime(task_info.start_time))
    , tp_end_(Int2Qtime(task_info.end_time))
    , cur_price_(0.0)
    , cur_state_(TaskCurrentState::STOP)
    , is_waitting_removed_(false)
{
    
}

bool StrategyTask::IsPriceJumpUp(double pre_price, double cur_price)
{
    if( cur_price < 0.01 || pre_price < 0.01 || cur_price < pre_price || Equal(cur_price, pre_price) ) 
        return false;
      
    if( pre_price < 10.0 ) return (cur_price - pre_price) > 0.06;
    else if( pre_price < 20.0 ) return (cur_price - pre_price) > 0.07;
    else if( pre_price < 50.0 ) return (cur_price - pre_price) > 0.08; 
    else if( pre_price < 80.0 ) return (cur_price - pre_price) > 0.09; 
    else if( pre_price < 100.0 ) return (cur_price - pre_price) > 0.1; 
    else if( pre_price < 150.0 ) return (cur_price - pre_price) > 0.15; 
    else if( pre_price < 200.0 ) return (cur_price - pre_price) > 0.3; 
    else  return (cur_price - pre_price) > 0.60;

}


bool StrategyTask::IsPriceJumpDown(double pre_price, double cur_price)
{
    return IsPriceJumpUp(cur_price, pre_price);
}

double StrategyTask::GetQuoteTargetPrice(const QuotesData& data, bool is_buy)
{
    switch(para_.target_price_level)
    {
    case TypeQuoteLevel::PRICE_CUR: return data.cur_price;
         
    case TypeQuoteLevel::PRICE_BUYSELL_1: return is_buy ? data.price_s_1 : data.price_b_1;
    case TypeQuoteLevel::PRICE_BUYSELL_2: return is_buy ? data.price_s_2 : data.price_b_2;
    case TypeQuoteLevel::PRICE_BUYSELL_3: return is_buy ? data.price_s_3 : data.price_b_3;
    case TypeQuoteLevel::PRICE_BUYSELL_4: return is_buy ? data.price_s_4 : data.price_b_4;
    case TypeQuoteLevel::PRICE_BUYSELL_5: return is_buy ? data.price_s_5 : data.price_b_5;
    default: return data.cur_price; 
    }
}

void StrategyTask::ObtainData(std::shared_ptr<QuotesData> &data)
{
    app_->task_calc_strand().PostTask([data, this]()
    {
        assert(data);
        if( quote_data_queue_.size() > cst_max_data_count)
        { 
            auto iter_end_del = quote_data_queue_.begin();
            for(int i = 0; i < cst_max_data_count/2; ++i )
                ++iter_end_del;
            quote_data_queue_.erase(quote_data_queue_.begin(), iter_end_del);
        }

        quote_data_queue_.push_back(std::move(data));

        // calc and judge if trigger trade
        HandleQuoteData();
    });

    if( !Equal(cur_price_, data->cur_price) ) 
    {
         cur_price_ = data->cur_price;
         app_->Emit(this, static_cast<int>(TaskStatChangeType::CUR_PRICE_CHANGE));
    }
    //std::chrono::system_clock::now().;
}
