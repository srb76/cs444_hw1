/*
 * elevator sstf
 * A modified version of noop-iosched.c
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

struct sstf_data {
	struct list_head queue;
	int head;
};

static void sstf_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	list_del_init(&next->queuelist);
}

static int sstf_dispatch(struct request_queue *q, int force)
{
	struct sstf_data *sd = q->elevator->elevator_data;
	char direction = 'Z';
	int sector = 0;
	if (!list_empty(&sd->queue)) {
		struct request *rq;
		rq = list_entry(sd->queue.next, struct request, queuelist);
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(q, rq);

		//Print if reading or writing at sector position
		
		if (rq_data_dir(rq) == 0)	//returns 0 for read, nonzero for write
			direction = 'R';
		else
			direction = 'W';

		sector = blk_rq_pos(rq);
		printk("----- IO dispatch: %c at sector %d.\n",direction,sector);
		
		//Save head position
		sd->head = sector;

		return 1;
	}
	return 0;
}

static void sstf_add_request(struct request_queue *q, struct request *rq)
{
	struct sstf_data *sd = q->elevator->elevator_data;
	/*
		C-LOOK logic

		Examine the sector of requests, if greater add to request queue

		list_add_tail to add to queue
		blk_rq_pos or rq_esd_sector? for checking sector position
	*/
	
	struct list_head *s;
	//struct request *head_req = list_entry();
	//int head = blk_rq_pos();
	
	//Get entry and determine position
	list_for_each(s, &sd->queue) {
		struct request *s_rq = list_entry(s, struct request, queuelist);
		
		if(blk_rq_pos(rq) >= sd->head) {
			//find queue position s where request belongs
			if(sd->head > blk_rq_pos(s_rq))
				break;	//stop when reaching sector before head
			else if(blk_rq_pos(s_rq) > blk_rq_pos(rq))
				break;	//stop when finding greater sector value
			//Otherwise continue navigating for each
		}
		else {	//Find position in sector before head
			if( (sd->head > blk_rq_pos(s_rq)) && (blk_rq_pos(s_rq) > blk_rq_pos(rq)) )
				break;
		}
	}
	int sector = 0;
	sector = blk_rq_pos(rq);	
	printk("----- IO add at sector %d.\n",sector);
	//Alway attempt to add after s index
	list_add_tail(&rq->queuelist, s);
}

static struct request *
sstf_former_request(struct request_queue *q, struct request *rq)
{
	struct sstf_data *sd = q->elevator->elevator_data;

	if (rq->queuelist.prev == &sd->queue)
		return NULL;
	return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *
sstf_latter_request(struct request_queue *q, struct request *rq)
{
	struct sstf_data *sd = q->elevator->elevator_data;

	if (rq->queuelist.next == &sd->queue)
		return NULL;
	return list_entry(rq->queuelist.next, struct request, queuelist);
}

static int sstf_init_queue(struct request_queue *q, struct elevator_type *e)
{
	struct sstf_data *sd;
	struct elevator_queue *eq;

	eq = elevator_alloc(q, e);
	if (!eq)
		return -ENOMEM;

	sd = kmalloc_node(sizeof(*sd), GFP_KERNEL, q->node);
	if (!sd) {
		kobject_put(&eq->kobj);
		return -ENOMEM;
	}
	eq->elevator_data = sd;

	INIT_LIST_HEAD(&sd->queue);

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);
	return 0;
}

static void sstf_exit_queue(struct elevator_queue *e)
{
	struct sstf_data *sd = e->elevator_data;

	BUG_ON(!list_empty(&sd->queue));
	kfree(sd);
}

static struct elevator_type elevator_sstf = {
	.ops = {
		.elevator_merge_req_fn		= sstf_merged_requests,
		.elevator_dispatch_fn		= sstf_dispatch,
		.elevator_add_req_fn		= sstf_add_request,
		.elevator_former_req_fn		= sstf_former_request,
		.elevator_latter_req_fn		= sstf_latter_request,
		.elevator_init_fn		= sstf_init_queue,
		.elevator_exit_fn		= sstf_exit_queue,
	},
	.elevator_name = "sstf",
	.elevator_owner = THIS_MODULE,
};

static int __init sstf_init(void)
{
	return elv_register(&elevator_sstf);
}

static void __exit sstf_exit(void)
{
	elv_unregister(&elevator_sstf);
}

module_init(sstf_init);
module_exit(sstf_exit);


MODULE_AUTHOR("Jens Axboe");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("No-op IO scheduler");
